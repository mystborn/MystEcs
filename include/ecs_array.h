#ifndef ECS_ECS_ARRAY_H
#define ECS_ECS_ARRAY_H

#include <stdio.h>

/*!
  \brief Ensures that the an array is large enough to have a specified index.

  \param array The array to potentially resize.
  \param current_size The current length of the array.
  \param new_size The index that the array must be large enough to support.
  \param element_size The size of the array element type.
 */
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

/*!
  \brief Resizes an array to be at least new_size big, and sets the value of all new elements to the specified default value.
         This macro won't work on arrays whose actual type is different than element_size.

  \param array The array to potentially resize.
  \param current_size The current length of the array.
  \param new_size The index that the array must be large enough to support.
  \param element_size The size of the array element type.
  \param default_value The default value to set the new elements of the array.
 */
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