record_mgr: record_mgr.o storage_mgr.o dberror.o test_assign3_1.o buffer_mgr.o buffer_mgr_stat.o expr.o rm_serializer.o
	cc -o record_mgr record_mgr.o storage_mgr.o dberror.o test_assign3_1.o buffer_mgr.o buffer_mgr_stat.o expr.o rm_serializer.o
record_mgr.o: record_mgr.c storage_mgr.h buffer_mgr.h record_mgr.h
	gcc -c record_mgr.c
storage_mgr.o: storage_mgr.c dberror.h storage_mgr.h  
	gcc -c storage_mgr.c
dberror.o: dberror.c dberror.h  
	gcc -c dberror.c
test_assign3_1.o: test_assign3_1.c dberror.h expr.h record_mgr.h tables.h test_helper.h
	gcc -c test_assign3_1.c
buffer_mgr.o: buffer_mgr.c buffer_mgr.h storage_mgr.h
	gcc -c buffer_mgr.c
buffer_mgr_stat.o: buffer_mgr_stat.c buffer_mgr_stat.h buffer_mgr.h
	gcc -c buffer_mgr_stat.c
expr.o: expr.c expr.h tables.h record_mgr.h dberror.h
	gcc -c expr.c
rm_serializer.o: rm_serializer.c dberror.h tables.h record_mgr.h
	gcc -c rm_serializer.c

