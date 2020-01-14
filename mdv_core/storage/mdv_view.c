#include "mdv_view.h"


mdv_view * mdv_view_retain(mdv_view *view)                  { return view->vptr->retain(view); }
uint32_t mdv_view_release(mdv_view *view)                   { return view ? view->vptr->release(view) : 0; }
mdv_table * mdv_view_desc(mdv_view *view)                   { return view->vptr->desc(view); }
mdv_rowset * mdv_view_fetch(mdv_view *view, size_t count)   { return view->vptr->fetch(view, count); }
