#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(SPOTIFY_QUASAR_LIB)
#  define SPOTIFY_QUASAR_EXPORT Q_DECL_EXPORT
# else
#  define SPOTIFY_QUASAR_EXPORT Q_DECL_IMPORT
# endif
#else
# define SPOTIFY_QUASAR_EXPORT
#endif
