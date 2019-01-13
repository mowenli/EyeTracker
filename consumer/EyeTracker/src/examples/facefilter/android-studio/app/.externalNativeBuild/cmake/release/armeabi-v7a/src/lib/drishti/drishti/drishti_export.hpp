
#ifndef DRISHTI_EXPORT_H
#define DRISHTI_EXPORT_H

#ifdef DRISHTI_STATIC_DEFINE
#  define DRISHTI_EXPORT
#  define DRISHTI_NO_EXPORT
#else
#  ifndef DRISHTI_EXPORT
#    ifdef drishti_EXPORTS
        /* We are building this library */
#      define DRISHTI_EXPORT 
#    else
        /* We are using this library */
#      define DRISHTI_EXPORT 
#    endif
#  endif

#  ifndef DRISHTI_NO_EXPORT
#    define DRISHTI_NO_EXPORT 
#  endif
#endif

#ifndef DRISHTI_DEPRECATED
#  define DRISHTI_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef DRISHTI_DEPRECATED_EXPORT
#  define DRISHTI_DEPRECATED_EXPORT DRISHTI_EXPORT DRISHTI_DEPRECATED
#endif

#ifndef DRISHTI_DEPRECATED_NO_EXPORT
#  define DRISHTI_DEPRECATED_NO_EXPORT DRISHTI_NO_EXPORT DRISHTI_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef DRISHTI_NO_DEPRECATED
#    define DRISHTI_NO_DEPRECATED
#  endif
#endif

#endif /* DRISHTI_EXPORT_H */
