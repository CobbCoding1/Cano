#ifndef COLORS_H
#define COLORS_H

// allows for custom colors to be defined throughout multipe files
// and share data between the two.

typedef struct {
    int custom_slot;
    int custom_id;
    int custom_r;
    int custom_g;
    int custom_b;
} Custom_Color;

#endif /* COLORS_H */