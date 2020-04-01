#include "CSocketServer.h"
#include <thread>

int main()
{
	CSocketServer Server{ 27000 };
	assert(Server.Initialize());
	Server.Listen();

	std::thread thr_receive
	{
		[&]() 
		{
			while (true)
			{
				if (Server.Receive() == false) break;
			}
		}
	};
	
	while (true)
	{
		if (GetAsyncKeyState(VK_ESCAPE) < 0) break;
	}

	thr_receive.detach();
	Server.ShutDownSending();
	return 0;
}