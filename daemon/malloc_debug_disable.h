
#ifdef __MALLOC_DEBUG__
#undef __MALLOC_DEBUG__

#ifdef MALLOC_DEBUG_LEVEL

#undef malloc
#undef calloc
#undef free

#endif /* MALLOC_DEBUG_LEVEL */

#endif /* __MALLOC_DEBUG__ */
