void foo() {
	// force format
	std::cout << "Hello, " << name() << "!";
}  // GCOV_EXCL_LINE

void bar() {
	foo();
	// GCOV_EXCL_START
	foo();
	foo();
	foo();
	// GCOV_EXCL_END
	foo();
	foo();
	foo();
	foo();
	// GCOV_EXCL_START
	foo();
	// GCOV_EXCL_STOP
	foo();
	foo();
}
