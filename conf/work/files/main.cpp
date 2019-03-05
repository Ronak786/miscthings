#include "hlog.h"
#include "watermark.h"

int main(int argc, char **argv)
{
	// this func call will loop forever

	cyn_info("\n\n++++ detectAndDrawWatermark ++++\n\n");

	detectAndDrawWatermark();

	return 0;
}

