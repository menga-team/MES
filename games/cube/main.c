#include <memory.h>
#include <stdbool.h>
#include "mesgraphics.h"
#include "timer.h"
#include "gpu.h"

static void vector3f_rotate(Vector3f *vec, float theta_x, float theta_y, float theta_z) {
        // rotate about x-axis
        vec->y = cosf(theta_x) * vec->y - sinf(theta_x) * vec->z;
        vec->z = sinf(theta_x) * vec->y + cosf(theta_x) * vec->z;
        // rotate about y-axis
        vec->x = cosf(theta_y) * vec->x + sinf(theta_y) * vec->z;
        vec->z = -sinf(theta_y) * vec->x + cosf(theta_y) * vec->z;
        // rotate about z-axis
        vec->x = cosf(theta_z) * vec->x - sinf(theta_z) * vec->y;
        vec->y = sinf(theta_z) * vec->x + cosf(theta_z) * vec->y;
}

int main(void) {
        RectangularBuffer buffer = buffer_create(160, 120);
        // clear the buffer
        memset(buffer.data, 0x00, BUFFER_SIZE(buffer.x, buffer.y));
        // draw a border
        buffer_draw_line(&buffer, 0, 0, 159, 0, 1);
        buffer_draw_line(&buffer, 0, 0, 0, 119, 1);
        buffer_draw_line(&buffer, 159, 0, 159, 119, 1);
        buffer_draw_line(&buffer, 0, 119, 160, 119, 1);
        // define the cube
        Vector3f corners[8] = {
                // front
                vector3f_create(40, 20, 100),
                vector3f_create(80, 20, 100),
                vector3f_create(40, 60, 100),
                vector3f_create(80, 60, 100),
                // back
                vector3f_create(40, 20, 150),
                vector3f_create(80, 20, 150),
                vector3f_create(40, 60, 150),
                vector3f_create(80, 60, 150),
        };
        // define which corners should be connected
        Vector2 lines[12] = {
                // front
                vector2_create(0, 1), // 40,20 -> 80,20
                vector2_create(0, 2), // 40,20 -> 40,60
                vector2_create(3, 1), // 80,60 -> 80,20
                vector2_create(3, 2), // 80,60 -> 40,60
                // back
                vector2_create(4, 5), // 40,20 -> 80,20
                vector2_create(4, 6), // 40,20 -> 40,60
                vector2_create(7, 5), // 80,60 -> 80,20
                vector2_create(7, 6), // 80,60 -> 40,60
                // connecting the squares
                vector2_create(0, 4),
                vector2_create(1, 5),
                vector2_create(2, 6),
                vector2_create(3, 7),
        };

        Vector3 old_corners[8];

        // initialize old_corners with current corners
        for (uint8_t i = 0; i < 8; ++i) {
                old_corners[i] = vector3f_round(&corners[i]);
        }

        while (true) {
                uint32_t start_time = millis_since_boot();
                // delete old lines
                for (uint8_t i = 0; i < 12; ++i) {
                        const Vector2 line = lines[i];
                        buffer_draw_line(&buffer, old_corners[line.x].x, old_corners[line.x].y,
                                         old_corners[line.y].x, old_corners[line.y].y, 0);
                }

                // calculate centroid
                Vector3f centroid = vector3f_create(0, 0, 0);
                for (uint8_t i = 0; i < 8; ++i) {
                        vector3_add(&centroid, &corners[i]);
                }
                vector3_divide(&centroid, 8);

                // apply velocity to corners & save for deleting later
                for (uint8_t i = 0; i < 8; ++i) {
                        vector3_subtract(&corners[i], &centroid);
                        vector3f_rotate(&corners[i], 0.005f, 0.005f, 0.005f);
                        vector3_add(&corners[i], &centroid);
                        Vector3 screen_position = vector3f_round(&corners[i]);
                        old_corners[i] = screen_position;
                }

                // draw lines connecting the corners
                for (uint8_t i = 0; i < 12; ++i) {
                        const Vector2 line = lines[i];
                        buffer_draw_line(&buffer, lroundf(corners[line.x].x), lroundf(corners[line.x].y),
                                         lroundf(corners[line.y].x), lroundf(corners[line.y].y), 1);
                }

                // send the frame and wait
                gpu_send_buf(FRONT_BUFFER, buffer.x, buffer.y, 0, 0, buffer.data);
                timer_block_ms(33 - (millis_since_boot() - start_time)); // 30fps
        }
}