## Overview

The Ocre SDK provides a standardized C header file (`ocre_api.h`) that defines the API for developing embedded applications for Ocre. This header-only SDK enables developers to write portable code that can run across different hardware platforms.

## Integration
This SDK is included as a submodule in the [getting-started](https://github.com/project-ocre/getting-started) repository. To get started with Ocre development:

```bash
git clone --recursive https://github.com/project-ocre/getting-started.git
```

For standalone projects, you can add this SDK as a submodule:

```bash
git submodule add https://github.com/project-ocre/ocre-sdk.git
```

## Usage
Include the SDK header in your application:
```c
#include "ocre-sdk/ocre_api.h"

int main() {
    // Use the API functions as defined in the header
    // Implementation provided by the target platform
    
    return 0;
}
```