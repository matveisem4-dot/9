#include <stdio.h>
#include <stdlib.h>
#include <xenos/xenos.h>
#include <console/console.h>
#include <input/input.h>
#include <timebase.h>

int main() {
    // Безопасная инициализация железа Xbox 360
    xenos_init(VIDEO_MODE_AUTO);
    console_init();
    input_init();

    // Настройка цветов: белый текст на темно-зеленом фоне
    console_set_colors(0xFFFFFFFF, 0xFF006400); 

    // Стартовые координаты
    float horse_x = 30.0f;
    float horse_y = 15.0f;
    float coin_x = 10.0f;
    float coin_y = 10.0f;
    int score = 0;

    while (1) {
        struct controller_data_s ctrl;

        // Читаем геймпад (порт 0)
        if (get_controller_data(&ctrl, 0)) {
            // Управление стиками или крестовиной
            if (ctrl.up) horse_y -= 0.5f;
            if (ctrl.down) horse_y += 0.5f;
            if (ctrl.left) horse_x -= 1.0f;
            if (ctrl.right) horse_x += 1.0f;

            // Ограничение движения по экрану
            if (horse_x < 0) horse_x = 0;
            if (horse_x > 60) horse_x = 60;
            if (horse_y < 0) horse_y = 30;
            if (horse_y > 30) horse_y = 30;
        }

        // Проверка сбора монеты (расстояние)
        float dx = horse_x - coin_x;
        float dy = horse_y - coin_y;
        if ((dx*dx + dy*dy) < 3.0f) {
            score += 10;
            // Генерация новой монеты через аппаратный таймер
            coin_x = (float)(mftb() % 50) + 5.0f;
            coin_y = (float)(mftb() % 25) + 2.0f;
        }

        // Отрисовка кадра
        console_clrscr();
        printf("\n\n   XBOX 360 HORSE GAME - SCORE: %d\n", score);
        printf("   =============================================\n\n");

        for (int y = 0; y < 25; y++) {
            printf("   "); // Отступ
            for (int x = 0; x < 60; x++) {
                if ((int)horse_y == y && (int)horse_x == x) {
                    printf("H"); // Игрок
                } else if ((int)coin_y == y && (int)coin_x == x) {
                    printf("O"); // Монетка
                } else {
                    printf("."); // Трава
                }
            }
            printf("\n");
        }

        printf("\n   Use D-PAD to run. Collect the 'O' coins!\n");

        // Задержка кадра для защиты от зависаний
        delay(30); 
    }

    return 0;
}
