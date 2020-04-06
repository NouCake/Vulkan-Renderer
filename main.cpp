#include <stdexcept>
#include <iostream>

#include "NouEngine.h"
//#include "MySecondVulkanApp.h"

int main() {

	try {
		//HelloTriangleApplication().run();
		NouEngine& e = *NouEngine::createInstance();
		e.run();
		delete& e;
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}