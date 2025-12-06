all: cproxy.c sproxy.c
	gcc cproxy.c -o cproxy
	gcc sproxy.c -o sproxy

clean:
	rm -f sproxy cproxy

move:
	mv cproxy venv/student_data_a
	mv sproxy venv/student_data_b
