
# Vector: A simple muteable array library for arduino. 

This implementation uses an underlying array to store its elements. When that array is filled the vector allocates a block of memory twice as large as its existing array. It then copies all the existing elements into that array and carries on. For that reason elements substituted as VectorType below need to implement a copy constructor and an operator= to facilitate that transfer if they're to be anything other than POD types.

To as greater extent as was practical Vector was designed to behave like a std::vector so for more information: http://www.cplusplus.com/reference/vector/vector/ is a good reference. Otherwise, for basic useage check /examples

NOTE: This library uses heap memory which can be problematic in microcontrollers where RAM is scarce. If memory availability is an issue then use Reserve(n) to allocate whatever is required at the beginning of the program and avoid pushing more than n elements during the program.

