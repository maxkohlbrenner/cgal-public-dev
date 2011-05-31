// ============================================================================
//
// Copyright (c) 2001-2008 Max-Planck-Institut Saarbruecken (Germany).
// All rights reserved.
//
// This file is part of EXACUS (http://www.mpi-inf.mpg.de/projects/EXACUS/);
// you may redistribute it under the terms of the Q Public License version 1.0.
// See the file LICENSE.QPL distributed with EXACUS.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// ----------------------------------------------------------------------------
//
// Library       : QdX
// File          : demos/xsurface/xsurface.C
// QdX_release   : $Name:  $
// Revision      : $Revision$
// Revision_date : $Date$
//
// Author(s)     : Pavel Emeliyanenko <asm@mpi-sb.mpg.de>
//
// ============================================================================

#include <CGAL/basic.h>
#include "include/mainwnd.h"

#if !1
#include <iostream>
int main( int, char**) {
    std::cerr << "This demo requires Qt!" << std::endl;
    return( 0);
}
#else

#include <qapplication.h>

#include <qgl.h>
#include <GL/gl.h>
#include <GL/glut.h>

#include <cstring>
#include <iostream>

int main (int argc, char **argv) {
    
    if(argc > 1 && std::strcmp(argv[1], "--test-suite") == 0) {
        std::cerr << "This interactive program terminates immediately for the "
                     "test-suite." << std::endl;
        return 0;
    }

    glutInit(&argc, argv);
    QApplication app(argc, argv);
  
    if(!QGLFormat::hasOpenGL()) {
        qWarning ("This system has no OpenGL support. Bailing out...");
        return 1;
    }
    
    XSurface_main_wnd *wnd = new XSurface_main_wnd(NULL, "Windows mustdie");
    app.setMainWidget(wnd);
    wnd->show();

    return app.exec();
}

#endif // QT
