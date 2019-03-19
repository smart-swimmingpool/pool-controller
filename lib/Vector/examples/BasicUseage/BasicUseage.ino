#include <Vector.h>

Vector<int> intVect;

void setup()
{
  Serial.begin(9600);
  
  int array[] = { 1, 2, 3, 4, 5 };
  
  // Fill the vector with the contents of the array
  intVect.Assign(array, 5);
  
  // Add another few elements to the array
  intVect.PushBack(6);
  intVect.PushBack(-300);
  intVect.PushBack(7);
  
  // Find the element with value -300
  int index = intVect.Find(-300);
  
  // Remove the element
  intVect.Erase(index);
  
  // Print off the results
  for(int i = 0; i < intVect.Size(); i++) 
    Serial.println(intVect[i]);
}

void loop()
{
}
