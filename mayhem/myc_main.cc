// Entry point for the fuzzed myc binary. upstream's main() strcpy()s argv[1] into a char[80]
// (myc/src/c.cpp:101), so with the long path Mayhem stages inputs under, EVERY input aborts there
// whatever its content. build.sh renames upstream's main to myc_main (-Dmain=myc_main, c.cpp only)
// and this main hands it a short-named copy of the input, so the fuzzer reaches the lexer, parser
// and VM instead. myc writes its <input>.s listing next to that name (and cuts the name at its FIRST
// dot), hence a private temp dir whose name has no dot.
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>

int myc_main(int argc, char *args[]);

static std::string tmpDir, tmpLink, tmpListing;

static void cleanup() {
    unlink(tmpLink.c_str());
    unlink(tmpListing.c_str());
    rmdir(tmpDir.c_str());
}

int main(int argc, char *argv[]) {
    if (argc < 2) return myc_main(argc, argv);
    const char *base = getenv("TMPDIR");
    std::string tmpl = std::string(base && *base ? base : "/tmp") + "/myc_XXXXXX";
    std::string buf = tmpl;
    if (!mkdtemp(&buf[0])) return myc_main(argc, argv);
    tmpDir = buf;
    tmpLink = tmpDir + "/in.c";
    tmpListing = tmpDir + "/in.s";
    FILE *in = fopen(argv[1], "rb");
    FILE *out = in ? fopen(tmpLink.c_str(), "wb") : nullptr;
    if (!out) {
        if (in) fclose(in);
        rmdir(tmpDir.c_str());
        return myc_main(argc, argv);
    }
    atexit(cleanup);
    char chunk[4096];
    size_t n;
    while ((n = fread(chunk, 1, sizeof chunk, in)) > 0) fwrite(chunk, 1, n, out);
    fclose(in);
    fclose(out);
    char *args[] = { argv[0], &tmpLink[0], nullptr };
    return myc_main(2, args);
}
