/****************************************************************************
   Amnuts UI builders — colour-aware horizontal rules, framed boxes,
   word-wrapped tables. See docs/superpowers/specs/2026-05-10-localisation-design.md §6.

   Function declarations live in src/includes/prototypes.h (single source
   of truth, per project convention).
 ***************************************************************************/

#ifndef AMNUTS_UIBUILDERS_H
#define AMNUTS_UIBUILDERS_H

#include <stddef.h>

/* Opaque handles. Defined in src/uibuilders.c. */
typedef struct box_struct   *BOX;
typedef struct table_struct *TABLE;

/* Alignment constants for align_into. */
enum align_value {
    ALIGN_LEFT   = 0,
    ALIGN_CENTRE = 1,
    ALIGN_RIGHT  = 2,
};

#endif /* AMNUTS_UIBUILDERS_H */
