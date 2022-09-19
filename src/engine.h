/* vim:set et sts=4: */
#ifndef __ENGINE_H__
#define __ENGINE_H__

#include <ibus.h>

#define IBUS_TYPE_BUGTEST_ENGINE	\
	(ibus_bugtest_engine_get_type ())

GType   ibus_bugtest_engine_get_type    (void);

#endif // __ENGINE_H__
