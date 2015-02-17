storage_mgr : storage_mgr.o test_assign1_1.o dberror.o
	cc -o storage_mgr dberror.o storage_mgr.o test_assign1_1.o 
dberror.o : dberror.c dberror.h
	cc -c -std=c99 dberror.c
storage_mgr.o : storage_mgr.c storage_mgr.h dberror.h
	cc -c -std=c99 storage_mgr.c
test_assign1_1.o : test_assign1_1.c test_helper.h storage_mgr.h dberror.h
	cc -c -std=c99 test_assign1_1.c
