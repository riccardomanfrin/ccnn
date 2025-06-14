#ifndef __GRAPH_OBJ_H__
#define __GRAPH_OBJ_H__

#include "point3d.h"
#include <stddef.h>

class GraphObj
{
public:
    virtual size_t points(Point3d *&points) const = 0;
};
#endif