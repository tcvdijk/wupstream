// On Windows, fopen wants different flags in our use case
#ifdef _WIN32
	const char *fopenMode = "rb";
#else
	const char *fopenMode = "r";
#endif
