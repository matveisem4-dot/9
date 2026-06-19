// Полноценная 3D игра про Лошадь для Xbox 360 (LibXenon Bare-Metal)

// Настройки экрана Xbox 360 (720p)
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define VIDEO_BASE 0x90000000 // Прямой доступ к видеопамяти Xbox 360

// Кнопки геймпада Xbox 360
#define PAD_UP    0x0001
#define PAD_DOWN  0x0002
#define PAD_LEFT  0x0004
#define PAD_RIGHT 0x0008
#define PAD_A     0x1000 // Стрельба

typedef struct { float x, y, z; } Vector3;
typedef struct { float x, z; int active; } Enemy;
typedef struct { float x, z; int active; } Bullet;

// Состояние игры
Vector3 horse_pos = {0.0f, 0.0f, 0.0f};
float horse_angle = 0.0f;

Vector3 coin_pos = {5.0f, 0.0f, 5.0f};
int score = 0;

#define MAX_ENEMIES 5
Enemy enemies[MAX_ENEMIES];

#define MAX_BULLETS 10
Bullet bullets[MAX_BULLETS];

// Крошечный генератор случайных чисел (так как stdlib.h недоступен)
static unsigned int rand_seed = 12345;
int pseudo_rand() {
    rand_seed = (rand_seed * 1103515245 + 12345) & 0x7fffffff;
    return rand_seed;
}

// 3D Математика: Синус и Косинус (Тейлор) для вращения и окружности арены
float get_sin(float x) {
    float x2 = x * x;
    return x * (1.0f - x2 * (1.0f / 6.0f - x2 * (1.0f / 120.0f)));
}
float get_cos(float x) {
    float x2 = x * x;
    return 1.0f - x2 * (0.5f - x2 * (1.0f / 24.0f - x2 * (1.0f / 720.0f)));
}

// Простая функция очистки экрана
void clear_screen(unsigned int color) {
    unsigned int *fb = (unsigned int *)VIDEO_BASE;
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        fb[i] = color;
    }
}

// Отрисовка пикселя с проверкой границ
void draw_pixel(int x, int y, unsigned int color) {
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
        unsigned int *fb = (unsigned int *)VIDEO_BASE;
        fb[y * SCREEN_WIDTH + x] = color;
    }
}

// 3D Проекция: превращает 3D координаты мира в 2D координаты экрана
int project_3d(Vector3 world, int *screen_x, int *screen_y) {
    // Камера смотрит сверху-сзади на лошадь
    float cam_x = horse_pos.x - get_sin(horse_angle) * 10.0f;
    float cam_z = horse_pos.z - get_cos(horse_angle) * 10.0f;
    float cam_y = 5.0f;

    float dx = world.x - cam_x;
    float dy = world.y - cam_y;
    float dz = world.z - cam_z;

    // Вращение относительно направления камеры
    float rx = dx * get_cos(-horse_angle) - dz * get_sin(-horse_angle);
    float rz = dx * get_sin(-horse_angle) + dz * get_cos(-horse_angle);

    if (rz <= 0.1f) return 0; // Объект позади камеры

    // Перспектива
    float fov = 400.0f;
    *screen_x = (int)(SCREEN_WIDTH / 2 + (rx * fov) / rz);
    *screen_y = (int)(SCREEN_HEIGHT / 2 + (dy * fov) / rz);
    return 1;
}

// Отрисовка 3D объекта как куба заданного цвета
void draw_3d_cube(Vector3 pos, float size, unsigned int color) {
    int sx, sy;
    if (project_3d(pos, &sx, &sy)) {
        int half_size = (int)(size * 20.0f); // Масштабирование от дистанции
        for (int x = sx - half_size; x < sx + half_size; x++) {
            for (int y = sy - half_size; y < sy + half_size; y++) {
                draw_pixel(x, y, color);
            }
        }
    }
}

// 3D Модель Лошади (собранная из полигональных блоков кодом)
void draw_horse() {
    // Тело лошади (Коричневый куб)
    draw_3d_cube(horse_pos, 1.2f, 0x8B4513FF);
    
    // Шея и голова
    Vector3 head_pos = {
        horse_pos.x + get_sin(horse_angle) * 0.8f,
        horse_pos.y + 0.6f,
        horse_pos.z + get_cos(horse_angle) * 0.8f
    };
    draw_3d_cube(head_pos, 0.6f, 0xA0522DFF);
}

// Точка входа в систему Xbox 360
void _start() {
    // Инициализация врагов
    for(int i = 0; i < MAX_ENEMIES; i++) {
        enemies[i].x = (float)(pseudo_rand() % 30 - 15);
        enemies[i].z = (float)(pseudo_rand() % 30 - 15);
        enemies[i].active = 1;
    }

    // Главный игровой цикл
    while (1) {
        // Чтение геймпада напрямую через регистры памяти (Заглушка для автономной сборки)
        unsigned int buttons = *(volatile unsigned int*)(0x20001000); 

        // 1. Управление лошадью
        if (buttons & PAD_LEFT)  horse_angle -= 0.05f;
        if (buttons & PAD_RIGHT) horse_angle += 0.05f;
        if (buttons & PAD_UP) {
            horse_pos.x += get_sin(horse_angle) * 0.1f;
            horse_pos.z += get_cos(horse_angle) * 0.1f;
        }
        if (buttons & PAD_DOWN) {
            horse_pos.x -= get_sin(horse_angle) * 0.1f;
            horse_pos.z -= get_cos(horse_angle) * 0.1f;
        }

        // Ограничение окружности арены (радиус 25 метров)
        float dist_from_center = horse_pos.x * horse_pos.x + horse_pos.z * horse_pos.z;
        if (dist_from_center > 25.0f * 25.0f) {
            // Отталкиваем назад в круг
            horse_pos.x -= get_sin(horse_angle) * 0.2f;
            horse_pos.z -= get_cos(horse_angle) * 0.2f;
        }

        // 2. Механика стрельбы (Кнопка А)
        if (buttons & PAD_A) {
            for(int i = 0; i < MAX_BULLETS; i++) {
                if(!bullets[i].active) {
                    bullets[i].x = horse_pos.x;
                    bullets[i].z = horse_pos.z;
                    bullets[i].active = 1;
                    break;
                }
            }
        }

        // Обновление пуль
        for(int i = 0; i < MAX_BULLETS; i++) {
            if(bullets[i].active) {
                bullets[i].x += get_sin(horse_angle) * 0.4f;
                bullets[i].z += get_cos(horse_angle) * 0.4f;
                
                // Проверка попадания во врагов
                for(int j = 0; j < MAX_ENEMIES; j++) {
                    if(enemies[j].active) {
                        float edx = bullets[i].x - enemies[j].x;
                        float edz = bullets[i].z - enemies[j].z;
                        if((edx*edx + edz*edz) < 1.5f) {
                            enemies[j].active = 0; // Враг убит
                            bullets[i].active = 0;
                        }
                    }
                }
            }
        }

        // 3. Сбор монеток
        float mdx = horse_pos.x - coin_pos.x;
        float mdz = horse_pos.z - coin_pos.z;
        if ((mdx * mdx + mdz * mdz) < 1.5f) {
            score++;
            coin_pos.x = (float)(pseudo_rand() % 20 - 10);
            coin_pos.z = (float)(pseudo_rand() % 20 - 10);
        }

        // РЕНДЕРИНГ (Максимум производительности Xbox 360)
        clear_screen(0x006400FF); // Зеленая трава (Задний фон)

        // Рисуем окружность арены (белые метки по краю)
        for(int a = 0; a < 360; a += 10) {
            float rad = (float)a * 0.0174f;
            Vector3 edge = {get_sin(rad) * 25.0f, 0.0f, get_cos(rad) * 25.0f};
            draw_3d_cube(edge, 0.3f, 0xFFFFFFFF);
        }

        // Рисуем монетку (Золотой куб)
        draw_3d_cube(coin_pos, 0.6f, 0xFFD700FF);

        // Рисуем врагов (Красные кубы людей)
        for(int i = 0; i < MAX_ENEMIES; i++) {
            if(enemies[i].active) {
                Vector3 ep = {enemies[i].x, 0.0f, enemies[i].z};
                draw_3d_cube(ep, 1.0f, 0xFF0000FF);
            }
        }

        // Рисуем пули (Синие вспышки)
        for(int i = 0; i < MAX_BULLETS; i++) {
            if(bullets[i].active) {
                Vector3 bp = {bullets[i].x, 0.2f, bullets[i].z};
                draw_3d_cube(bp, 0.2f, 0x0000FFFF);
            }
        }

        // Рисуем саму Лошадь
        draw_horse();

        // Небольшая задержка кадров
        for (volatile int i = 0; i < 400000; i++);
    }
}
