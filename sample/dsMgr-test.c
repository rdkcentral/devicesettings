
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include "exception.hpp"
#include "host.hpp"
#include "manager.hpp"


int main(int argc, char *argv[])
{
    printf("%d:%s: enter\n", __LINE__, __func__);
	try {
            device::Manager::Initialize();
        } catch (const device::Exception& e) {
            printf("Exception caught %s", e.what());
        } catch (const std::exception& e) {
            printf("Exception caught %s", e.what());
        } catch (...) {
            printf("Exception caught unknown");
        }
	printf("%d:%s: Exit\n", __LINE__, __func__);
    return 0;
}




/** @} */
/** @} */
