set args -V -f ./traces/realloc2-bal.rep
#b * mm_init
#b * mm_malloc
b * mm_realloc
#b * mm_free 
#b * place
#b * coalesced
#b * extend_heap
start 
