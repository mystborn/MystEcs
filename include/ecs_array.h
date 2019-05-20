#ifndef ECS_ECS_ARRAY_H
#define ECS_ECS_ARRAY_H

#include <stdio.h>

// Resizes an array to be at least new_size big.
// array: The array to resize.
// current_size: The amount of elements that can currently be safely be added to the array.
// new_size: The size that the array must be big enough to support.
// element_size: The size of the array element type.
#define ECS_ARRAY_RESIZE(array, current_size, new_size, element_size) \
    do { \
        if((current_size) <= (new_size)) { \
            while((new_size) >= (current_size)) { \
                if((current_size) == 0) {\
                    (current_size) = 4; \
                } else {\
                    (current_size) *= 2;\
                }\
            } \
            (array) = ecs_realloc((array), (element_size) * (current_size)); \
        } \
    } while(0)

// Resizes an array to be at least new_size big, and sets the value of all new elements to the specified default value.
// array: The array to resize.
// current_size: The amount of elements that can currently be safely be added to the array.
// new_size: The size that the array must be big enough to support.
// element_size: The size of the array element type.
// default_value: The default value to set the new elements of the array.
#define ECS_ARRAY_RESIZE_DEFAULT(array, current_size, new_size, element_size, default_value) \
    do { \
        if((current_size) <= (new_size)) { \
            int ___ecs_array_old_size = current_size; \
            while((new_size) >= (current_size)) { \
                if((current_size) == 0) {\
                    (current_size) = 4; \
                } else {\
                    (current_size) *= 2;\
                }\
            } \
            (array) = ecs_realloc((array), (element_size) * (current_size)); \
            while(___ecs_array_old_size < (current_size)) *((array) + ___ecs_array_old_size++) = (default_value); \
        } \
    } while(0)

#endif