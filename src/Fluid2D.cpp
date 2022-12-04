#include "Fluid2D.h"
#include "GLHeaders.h"
#include <future>
#include <limits>

struct TicTok {
    const char *label;
    std::chrono::steady_clock::time_point start;

    explicit TicTok(const char *l) : label(l) {
        start = std::chrono::steady_clock::now();
    }

    ~TicTok() {
        std::cout << "\r[" << label << "] cost : "
                  << (std::chrono::steady_clock::now() - start).count() * 1e-6;

    }
};

Fluid2D::Fluid2D(Fluid2DParameters &params) {
    this->params = params;
    this->scale = 1;
    pool = new nano_std::ThreadPool(std::thread::hardware_concurrency());
    init();
}

void Fluid2D::init() {
    // alloc memory
    positions.resize(params.particle_count);
    back_positions.resize(params.particle_count);
    velocities.clear();
    acc_s.clear();
    acc_s.resize(params.particle_count);
    velocities.resize(params.particle_count);
    grid_raw = int(std::floor((params.top - params.bottom) / params.h)) + 1;
    grid_col = int(std::floor((params.right - params.left) / params.h)) + 1;
    grid.resize(grid_raw * grid_col);

    // init positions
    if (params.init_positions != nullptr) {
        params.init_positions(positions, params.top, params.bottom, params.left, params.right);
        back_positions = positions;
    }
}

void Fluid2D::update() {
    render();
}

void Fluid2D::start() {
    // main dispatch task
    this->is_running = true;
    // initial acceleration
    acc_s.clear();
    acc_s.resize(params.particle_count);
    acceleration(positions, velocities, acc_s);
    function<void()> task = [this]() {
        while (this->is_running) {
            TicTok t("one step duration");
            this->step();
            std::unique_lock<std::mutex> lk(this->swap_mutex);
            this->back_positions = this->positions;
        }
    };
    dispatcher.insertTask(task);
    dispatcher.run();
}

void Fluid2D::resetWithCallback(std::function<void()> callback) {
    stop();
    std::this_thread::sleep_for(std::chrono::milliseconds (100));
    pool->doSync([this, &callback]() {
        init();
        callback();
        // update boundary
        for (auto &boundary:boundaries) {
            boundary->updateCS(params.top, params.bottom, params.right , params.left);
        }
    });
}

void Fluid2D::step() {
    index_all_particles();
    // leap frogs
    std::vector<vec2 > velocity_half(params.particle_count);
    float half_dt = params.delta_t / 2;
    for (int i = 0; i < params.particle_count; i++) {
        vec2 dv = acc_s[i] * half_dt;
        velocity_half[i] = velocities[i] + dv;
        vec2 dp = velocity_half[i] * params.delta_t;
        // update position and boundary check
        vec2 next_position = positions[i] + dp;
        // boundaries check
        bool should_update_pos = true;
        for (auto &boundary:boundaries) {
            if (boundary->updateAt(i, next_position, positions, velocity_half)) {
                should_update_pos = false;
            }
        }
        // try to update positions
        if (should_update_pos) {
            positions[i] = next_position;
        }
        // grid boundary
        update_boundary(i, positions, velocity_half);
    }
    // update velocities
    std::vector<vec2 > next_acc(params.particle_count);
    acceleration(positions, velocity_half, next_acc);
    for (int i = 0; i < params.particle_count; i++) {
        vec2 dv = next_acc[i] * half_dt;
        velocities[i] = velocity_half[i] + dv;
    }
    // update acc
    acc_s = next_acc;
}

void Fluid2D::index_all_particles() {
    // index all particles into grid;
    float grid_bottom = params.bottom - params.h / 2;
    float grid_left = params.left - params.h / 2;
    for (auto &cell: grid) {
        cell.clear();
    }
    int p_index = 0;
    for (vec2 &p: positions) {
        float dy = p.y() - grid_bottom;
        float dx = p.x() - grid_left;
        // boundary check
        if (dx > 0 && dy > 0) {
            int col_index = ::floor(dx / params.h);
            int raw_index = ::floor(dy / params.h);
            // insert into grid;
            cellAt(col_index, raw_index).push_back(p_index++);
        }
    }
}

void Fluid2D::acceleration(const std::vector<vec2 > &position,
                           const std::vector<vec2 > &velocity,
                           std::vector<vec2 > &acc) {
    // foreach grid cell, calculate all neighbours
    std::vector<std::vector<int> > all_groups(grid_col * grid_raw);
    std::vector<float> pho(params.particle_count);
    std::vector<function<void(void)>> tasks;
    for (int i = 0; i < grid_raw; i++) {
        for (int j = 0; j < grid_col; j++) {
            // all particles indices in a 3 x 3 grid whose center is cell
            vector<int> group;
            for (int k = -1; k < 2; k++) {
                for (int d = -1; d < 2; d++) {
                    if (inGrid(j + k, i + d)) {
                        for (int index: cellAt(j + k, i + d)) {
                            group.push_back(index);
                        }
                    }
                }
            }
            all_groups[i * grid_col + j] = group;
        }
    }
    // calculate pho
    for (int i = 0; i < grid_raw; i++) {
        for (int j = 0; j < grid_col; j++) {
            for (int particle: cellAt(j, i)) {
                tasks.emplace_back([&position, &pho, particle, i, j, this, &all_groups]() {
                    float p = 0;
                    vec2 pos = position[particle];
                    for (int other: all_groups[i * grid_col + j]) {
                        if (!isSeperatedByBoundaries(particle, other, position)) {
                            vec2 other_pos = position[other];
                            vec2 dr = pos - other_pos;
                            p = p + params.particle_mass * (*params.rho_kernel)(dr, params.h);
                        }
                    }
                    pho[particle] = p;
                });
            }
        }
    }
    pool->syncGroup(tasks, tasks.size() / 200);

    // get acceleration
    tasks.clear();
    for (int i = 0; i < grid_raw; i++) {
        for (int j = 0; j < grid_col; j++) {
            // for all particle in the cell, calculate all acceleration
            if (cellAt(j, i).size() > 0) {
                // get color field gradient
                vec2 n;
                vec2 cell_center(j + 0.5, i + 0.5);
                for (int k = -1; k < 2; k++) {
                    for (int d = -1; d < 2; d++) {
                        if (!(k == 0 && d == 0) && inGrid(j + k, i + d)) {
                            vec2 other_center(j + k + 0.5, i + d + 0.5);
                            if (cellAt(j + k, i + d).empty() || isSeperatedByBoundaries(cell_center, other_center)) {
                                n.x() -= float(k);
                                n.y() -= float(d);
                            }
                        }
                    }
                }
                for (int particle: cellAt(j, i)) {
                    tasks.emplace_back([this, particle, i, j, &all_groups, n, &position, &velocity, &pho, &acc]() {
                        if (!this->is_running) { return; }
                        acceleration_at(particle,
                                        n,
                                        all_groups[i * grid_col + j],
                                        position,
                                        velocity,
                                        pho,
                                        acc);
                    });
                }
            }
        }
    }
    pool->syncGroup(tasks, params.particle_count / 200);
}

void Fluid2D::acceleration_at(int p_index,
                              vec2 surf_n,
                              const std::vector<int> &neighbours,
                              const std::vector<vec2 > &position,
                              const std::vector<vec2 > &velocity,
                              const std::vector<float> &pho_s,
                              std::vector<vec2 > &acc) {
    // key function, calculate all accelerations
    //* external forces: */
    /// gravity
    vec2 ac = params.gravity;
    vec2 pos = position[p_index];
    vec2 vel = velocity[p_index];
    float rho_p = pho_s[p_index];
    float rho_p_2 = rho_p * 2;
    float pr = params.K * (rho_p - params.rho_0);

    /* internal force */
    if (params.pressure_kernel != nullptr) {
        for (int other: neighbours) {
            if (other != p_index && !isSeperatedByBoundaries(p_index, other, position)) {
                vec2 other_pos = position[other];
                vec2 dr = pos - other_pos;
                /* pressure */
                // f_pressure = - m * (p_i +p_j) / (2 * pho_i) * diff_W(r, h)
                // a_pressure = f / m = - (p_i +p_j) / (2 * pho_i) * diff_W(r, h)
                // p = K * (pho - pho_0)
                if (params.pressure_kernel != nullptr) {
                    vec2 pressure = params.pressure_kernel->diff(dr, params.h) *
                                    (-1 * (params.K * (pho_s[other] - params.rho_0) + pr) / rho_p_2);
                    ac = ac + pressure;
                }
                /* viscosity */
                // f_viscosity = miu * m * (vj - vi) / pho_j * laplace_W(r, h)
                // a_viscosity = miu * (vj - vi) / pho_j * laplace_W(r, h)
                if (params.viscosity_kernel != nullptr) {
                    vec2 d_v = velocity[other];
                    d_v = d_v - vel;
                    vec2 viscosity =
                            d_v * (params.viscosity_kernel->laplace(dr, params.h) * params.V / pho_s[other]);
                    ac = ac + viscosity;
                }

                /* surface tension */
                // f_tension = sigma * kappa * normal
                // kappa = - m * laplace_W(r, h) / (pho_j * |normal|)
                if (params.surface_tension_kernel != nullptr) {
                    float norm = surf_n.length();
                    if (norm > std::numeric_limits<float>::epsilon()) {
                        float kappa = -params.surface_tension_kernel->laplace(dr, params.h) / (pho_s[other] * norm);
                        vec2 tension = surf_n * (kappa * params.sigma);
                        ac = ac + tension;
                    }
                }

            }
        }
    }

    acc[p_index] = ac;
}

void Fluid2D::update_boundary(int p_index, std::vector<vec2 > &position, std::vector<vec2 > &velocity) const {
    vec2 pos = position[p_index];
    if (pos.x() <= params.left) {
        position[p_index].x() = params.left + 0.1f;
        velocity[p_index].x() = 0.1f * std::abs(velocity[p_index].x());
    } else if (pos.x() >= params.right) {
        position[p_index].x() = params.right - 0.1f;
        velocity[p_index].x() = - 0.1f *  std::abs(velocity[p_index].x());
    }
    if (pos.y() <= params.bottom) {
        position[p_index].y() = params.bottom + 0.1f;
        velocity[p_index].y() = 0.1f * std::abs(velocity[p_index].y());
    } else if (pos.y() >= params.top) {
        position[p_index].y() = params.top - 0.1f;
        velocity[p_index].y() = -0.1f * std::abs(velocity[p_index].y());
    }
}

void Fluid2D::render() {
    glScalef(scale, scale, scale);
    glColor3f(0.3, 0.5, 0.8);
    glPointSize(2);
    glBegin(GL_POINTS);
    std::unique_lock<std::mutex> lk(this->swap_mutex);
    std::vector<vec2 > lock_positions = back_positions;
    lk.unlock();
    float center_x = (params.left + params.right) / 2;
    float center_y = (params.top + params.bottom) / 2;
    for (vec2 &p: lock_positions) {
        glVertex3f(p.x() - center_x, p.y() - center_y, 0);
    }
    glEnd();
    // render grid
    glColor3f(0.1, 0.1, 0.1);
    glLineWidth(1);
    glBegin(GL_LINES);
    float grid_w = (params.right - params.left) / float(grid_col);
    float grid_h = (params.top - params.bottom) / float(grid_raw);
    for (unsigned int x = 0; x <= grid_col; x++) {
        glVertex3f(-center_x + grid_w * x, params.top - center_y, 0);
        glVertex3f(-center_x + grid_w * x, params.bottom - center_y, 0);
    }
    for (unsigned int y = 0; y <= grid_raw; y++) {
        glVertex3f(params.left - center_x, -center_y + grid_h * y, 0);
        glVertex3f(params.right - center_x, -center_y + grid_h * y, 0);
    }
    glEnd();
}

Fluid2D::~Fluid2D() {
    this->stop();
    dispatcher.stop();
    delete pool;
}

