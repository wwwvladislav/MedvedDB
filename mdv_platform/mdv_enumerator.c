#include "mdv_enumerator.h"


mdv_enumerator * mdv_enumerator_retain(mdv_enumerator *enumerator)  { return enumerator->vptr->retain(enumerator); }
uint32_t mdv_enumerator_release(mdv_enumerator *enumerator)         { return enumerator ? enumerator->vptr->release(enumerator) : 0; }
mdv_errno mdv_enumerator_reset(mdv_enumerator *enumerator)          { return enumerator->vptr->reset(enumerator); }
mdv_errno mdv_enumerator_next(mdv_enumerator *enumerator)           { return enumerator->vptr->next(enumerator); }
void * mdv_enumerator_current(mdv_enumerator *enumerator)           { return enumerator->vptr->current(enumerator); }
