#include <Vector.h>

// ForEach is a slightly more efficient version of:

  // for(int i = 0; i < Size(); i++)
    // do whatever
    
// To use it you need to subclass Predicate and define its () operator like so:
class PrintOffPredicate : public Predicate<int>
{
    void operator() (int &element) { Serial.println(element); }
} printOff;

Vector<int> intVect;

void setup()
{
  Serial.begin(9600);
  
  int array[] = { 1, 1, 2, 3, 5, 8 };
  
  // Fill the vector with the contents of the array
  intVect.Assign(array, 6);
  
  // Print off the results
  intVect.ForEach(printOff);
}

void loop()
{
}
