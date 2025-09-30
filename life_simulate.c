#pragma warning(push, 0)
#include <stdint.h>

#define WIN32_LEAN_AND_MEAN
#define UNICODE
#include <windows.h>
#pragma(pop)

#pragma warning(disable :5045)

typedef struct
{
    uint32_t x;
    uint32_t y;
} start_coord_t;

#include <stdlib.h>


typedef uint8_t   u8 ;
typedef uint32_t  u32;

#define THREAD_COUNT 16

typedef struct
{
    u32  begin_x;
    u32  begin_y;
    u32  end_x;
    u32  end_y;
    u32  grid_size;
    volatile u8 finished;
    volatile u8 new_work;
} work_t;

typedef struct
{
    HANDLE threads[THREAD_COUNT];
    work_t work[THREAD_COUNT];
    u8 *input;
    u8 *staging;
    u8 *output;
} state_t;

state_t state = {0};

void work_thread(void *argument)
{
    work_t *work = (work_t *)argument;

    while(1)
    {
        while (!work->new_work);
        for (int i = work->begin_x; i < work->end_x; i++)
        {
            for (int j = work->begin_y; j < work->end_y; j++)
            {
                int neighbour_count = 0;
                for (int y1 = i - 1; y1 <= i + 1; y1++)
                    for (int x1 = j - 1; x1 <= j + 1; x1++)
                        if (state.input[(x1 + work->grid_size) % work->grid_size + ((y1 + work->grid_size) % work->grid_size) * work->grid_size])
                            neighbour_count++;
                uint8_t current_state = state.input[j + i * work->grid_size];
                if (current_state)
                    neighbour_count--;
                state.staging[j + i * work->grid_size] = (neighbour_count == 3 || (neighbour_count == 2 && current_state));
            }
        }
        work->grid_size = 0;
        work->new_work = 0;
        work->finished = 1;
    }
}

uint8_t *simulate_life(uint32_t grid_dim, start_coord_t *initial_points, uint32_t initial_point_count)
{
    if (!state.output)
    {
        state.output = malloc(sizeof(uint8_t) * grid_dim * grid_dim * 3);
        state.input = state.output + sizeof(uint8_t) * grid_dim * grid_dim;
        state.staging = state.input + sizeof(uint8_t) * grid_dim * grid_dim;
        memset(state.output, 0, sizeof(uint8_t) * grid_dim * grid_dim * 3);

        for (uint32_t initial_points_index = 0;  initial_points_index < initial_point_count; initial_points_index++)
        {
            state.input[initial_points[initial_points_index].x + initial_points[initial_points_index].y * grid_dim] = 1;
        }
        memcpy(state.output, state.input, sizeof(uint8_t) * grid_dim * grid_dim);
        u32 chunk_size_x = grid_dim;
        u32 chunk_size_y = grid_dim / THREAD_COUNT;
        u32 grid_y = 0;
        u32 grid_x = 0;

        u32 work_idx = 0;
        for (u32 grid_y = 0; grid_y < grid_dim; grid_y += chunk_size_y)
        {
            for (u32 grid_x = 0; grid_x < grid_dim; grid_x += chunk_size_x)
            {
                state.work[work_idx] = (work_t){.begin_x = grid_x, .begin_y = grid_y, .end_x = grid_x + chunk_size_x, .end_y = grid_y + chunk_size_y, .grid_size = grid_dim, .new_work = 1};
                work_idx++;
            }
        }

        for (u32 thread_idx = 0; thread_idx < THREAD_COUNT; thread_idx++)
        {
            state.threads[thread_idx] = CreateThread(NULL, 1024, work_thread, &state.work[thread_idx], 0, NULL);
        }
    }
    else
    {
        for (u32 i = 0; i < THREAD_COUNT; i++)
        {
            while(!state.work[i].finished);
        }
        memcpy(state.output, state.staging, sizeof(uint8_t) * grid_dim * grid_dim);
        memcpy(state.input, state.staging, sizeof(uint8_t) * grid_dim * grid_dim);
        u32 chunk_size_x = grid_dim;
        u32 chunk_size_y = grid_dim / THREAD_COUNT;

        u32 grid_y = 0;
        u32 grid_x = 0;

        u32 work_idx = 0;
        for (u32 grid_y = 0; grid_y < grid_dim; grid_y += chunk_size_y)
        {
            for (u32 grid_x = 0; grid_x < grid_dim; grid_x += chunk_size_x)
            {
                state.work[work_idx] = (work_t){.begin_x = grid_x, .begin_y = grid_y, .end_x = grid_x + chunk_size_x, .end_y = grid_y + chunk_size_y, .grid_size = grid_dim, .new_work = 1};
                work_idx++;
            }
        }
    }
    return state.output;
}
