/*主文件，包含main函数*/
#include <string>
#include <iostream>
#include "../../common/type.h"
#include "../../common/local_log.h"
#include "google_bid.h"
#include "google_context.h"

using namespace std;


BidContext *g_context = NULL;


int main()
{
	cout << "google fast cgi start\n";
	g_context = new GoogleContext;
	if (!g_context->Init(PRIVATE_CONF))
	{
		cout << "g_context->Init failed\n";
		return 1;
	}

	g_context->StartWork();
	g_context->Monitor();

	cout << "g_context->Uninit()" << endl;
	g_context->Uninit();
	cout << "end\n";
	return 0;
}
