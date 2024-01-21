#ifndef COLORS_H
#define COLORS_H

// allows for custom colors to be defined throughout multipe files
// and share data between the two.

typedef enum { 
    YELLOW_COLOR = 1,
    BLUE_COLOR,
    GREEN_COLOR,
    RED_COLOR,
    CYAN_COLOR,
    MAGENTA_COLOR,
} Color_Pairs;

typedef struct {
    Color_Pairs custom_slot;
    int custom_id;
    int custom_r;
    int custom_g;
    int custom_b;
} Custom_Color;

#endif /* COLORS_H */
