/****************************************************************************
   Amnuts UI builders — colour-aware horizontal rules, framed boxes,
   word-wrapped tables. See docs/superpowers/specs/2026-05-10-localisation-design.md §6.

   Function declarations live in src/includes/prototypes.h (single source
   of truth, per project convention).

   The ALIGN_LEFT / ALIGN_CENTRE / ALIGN_RIGHT constants are macros in
   defines.h (shared with the pre-existing align_string function).
 ***************************************************************************/

#ifndef AMNUTS_UIBUILDERS_H
#define AMNUTS_UIBUILDERS_H

#include <stddef.h>

/* Opaque handles. Defined in src/uibuilders.c. */
typedef struct box_struct   *BOX;
typedef struct table_struct *TABLE;

#endif /* AMNUTS_UIBUILDERS_H */
