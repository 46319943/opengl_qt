/************************************************************************/
/*** END OF CODE SECTION TRIANGULATE.CPP BEGINNING OF TEST.CPP A SMALL **/
/*** TEST APPLICATION TO DEMONSTRATE THE USAGE OF THE TRIANGULATOR     **/
/************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


#include "triangulate.h"

void main(int argc,char **argv)
{

  // Small test application demonstrating the usage of the triangulate
  // class.


  // Create a pretty complicated little contour by pushing them onto
  // an stl vector.

  Vector2dVector a;

  a.push_back( Vector2d(0,6));
  a.push_back( Vector2d(0,0));
  a.push_back( Vector2d(3,0));
  a.push_back( Vector2d(4,1));
  a.push_back( Vector2d(6,1));
  a.push_back( Vector2d(8,0));
  a.push_back( Vector2d(12,0));
  a.push_back( Vector2d(13,2));
  a.push_back( Vector2d(8,2));
  a.push_back( Vector2d(8,4));
  a.push_back( Vector2d(11,4));
  a.push_back( Vector2d(11,6));
  a.push_back( Vector2d(6,6));
  a.push_back( Vector2d(4,3));
  a.push_back( Vector2d(2,6));

  // allocate an STL vector to hold the answer.

  Vector2dVector result;

  //  Invoke the triangulator to triangulate this polygon.
  Triangulate::Process(a,result);

  // print out the results.
  int tcount = result.size()/3;

  for (int i=0; i<tcount; i++)
  {
    const Vector2d &p1 = result[i*3+0];
    const Vector2d &p2 = result[i*3+1];
    const Vector2d &p3 = result[i*3+2];
    printf("Triangle %d => (%0.0f,%0.0f) (%0.0f,%0.0f) (%0.0f,%0.0f)\n",i+1,p1.GetX(),p1.GetY(),p2.GetX(),p2.GetY(),p3.GetX(),p3.GetY());
  }

}
