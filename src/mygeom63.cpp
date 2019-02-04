/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Tesselated Polygon Object
 * Author:   David Register
 *
 ***************************************************************************
 *   Copyright (C) 2010 by David S. Register   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  USA.             *
 ***************************************************************************
 *
 *
 */
// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
#include "wx/wx.h"
#endif //precompiled headers

#include "wx/tokenzr.h"
#include <wx/mstream.h>


#include "mygeom63.h"

#include <math.h>
#include "triangulate.h"

#ifdef ocpnUSE_GL

#ifdef USE_GLU_TESS
#ifdef __WXOSX__
#include "GL/gl.h"
#include "GL/glu.h"
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#endif

#ifdef __WXMSW__
#include <windows.h>
#endif

#endif

//------------------------------------------------------------------------------
//          Some local definitions for opengl/glu types,
//            just enough to build the glu tesselator option.
//          Included here to avoid having to find and include
//            the Microsoft versions of gl.h and glu.h.
//          You are welcome.....
//------------------------------------------------------------------------------
/*
#ifdef __WXMSW__
class GLUtesselator;

typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef signed char GLbyte;
typedef short GLshort;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned int GLuint;
typedef float GLfloat;
typedef float GLclampf;
typedef double GLdouble;
typedef double GLclampd;
typedef void GLvoid;

#define GLU_TESS_BEGIN                     100100
#define GLU_TESS_VERTEX                    100101
#define GLU_TESS_END                       100102
#define GLU_TESS_ERROR                     GLU_ERROR
#define GLU_TESS_EDGE_FLAG                 100104
#define GLU_TESS_COMBINE                   100105
#define GLU_TESS_BEGIN_DATA                100106
#define GLU_TESS_VERTEX_DATA               100107
#define GLU_TESS_END_DATA                  100108
#define GLU_TESS_ERROR_DATA                100109
#define GLU_TESS_EDGE_FLAG_DATA            100110
#define GLU_TESS_COMBINE_DATA              100111
#define GLU_BEGIN                          GLU_TESS_BEGIN
#define GLU_VERTEX                         GLU_TESS_VERTEX
#define GLU_END                            GLU_TESS_END
#define GLU_EDGE_FLAG                      GLU_TESS_EDGE_FLAG
#define GLU_CW                             100120
#define GLU_CCW                            100121
#define GLU_INTERIOR                       100122
#define GLU_EXTERIOR                       100123
#define GLU_UNKNOWN                        100124
#define GLU_TESS_WINDING_RULE              100140
#define GLU_TESS_BOUNDARY_ONLY             100141
#define GLU_TESS_TOLERANCE                 100142
#define GLU_TESS_ERROR1                    100151
#define GLU_TESS_ERROR2                    100152
#define GLU_TESS_ERROR3                    100153
#define GLU_TESS_ERROR4                    100154
#define GLU_TESS_ERROR5                    100155
#define GLU_TESS_ERROR6                    100156
#define GLU_TESS_ERROR7                    100157
#define GLU_TESS_ERROR8                    100158
#define GLU_TESS_MISSING_BEGIN_POLYGON     100151
#define GLU_TESS_MISSING_BEGIN_CONTOUR     100152
#define GLU_TESS_MISSING_END_POLYGON       100153
#define GLU_TESS_MISSING_END_CONTOUR       100154
#define GLU_TESS_COORD_TOO_LARGE           100155
#define GLU_TESS_NEED_COMBINE_CALLBACK     100156
#define GLU_TESS_WINDING_ODD               100130
#define GLU_TESS_WINDING_NONZERO           100131
#define GLU_TESS_WINDING_POSITIVE          100132
#define GLU_TESS_WINDING_NEGATIVE          100133
#define GLU_TESS_WINDING_ABS_GEQ_TWO       100134

#define GL_TRIANGLES                       0x0004
#define GL_TRIANGLE_STRIP                  0x0005
#define GL_TRIANGLE_FAN                    0x0006

#endif
*/

//      Module Internal Prototypes


#ifdef USE_GLU_TESS
static int            s_nvcall;
static int            s_nvmax;
static double         *s_pwork_buf;
static int            s_buf_len;
static int            s_buf_idx;
static unsigned int   s_gltri_type;
TriPrim               *s_pTPG_Head;
TriPrim               *s_pTPG_Last;
static GLUtesselator  *GLUtessobj;
static double         s_ref_lat;
static double         s_ref_lon;

static bool           s_bmerc_transform;
static double         s_transform_x_rate;
static double         s_transform_x_origin;
static double         s_transform_y_rate;
static double         s_transform_y_origin;
wxArrayPtrVoid        *s_pCombineVertexArray;

static const double   CM93_semimajor_axis_meters = 6378388.0;            // CM93 semimajor axis

#endif


int s_idx;


//  For __WXMSW__ builds using GLU_TESS and glu32.dll
//  establish the dll entry points

#ifdef __WXMSW__
#ifdef USE_GLU_TESS
#ifdef USE_GLU_DLL

//  Formal definitions of required functions
typedef void (CALLBACK* LPFNDLLTESSPROPERTY)      ( GLUtesselator *tess,
                                                    GLenum        which,
                                                    GLdouble      value );
typedef GLUtesselator * (CALLBACK* LPFNDLLNEWTESS)( void);
typedef void (CALLBACK* LPFNDLLTESSBEGINCONTOUR)  ( GLUtesselator *);
typedef void (CALLBACK* LPFNDLLTESSENDCONTOUR)    ( GLUtesselator *);
typedef void (CALLBACK* LPFNDLLTESSBEGINPOLYGON)  ( GLUtesselator *, void*);
typedef void (CALLBACK* LPFNDLLTESSENDPOLYGON)    ( GLUtesselator *);
typedef void (CALLBACK* LPFNDLLDELETETESS)        ( GLUtesselator *);
typedef void (CALLBACK* LPFNDLLTESSVERTEX)        ( GLUtesselator *, GLdouble *, GLdouble *);
typedef void (CALLBACK* LPFNDLLTESSCALLBACK)      ( GLUtesselator *, const int, void (CALLBACK *fn)() );

//  Static pointers to the functions
static LPFNDLLTESSPROPERTY      s_lpfnTessProperty;
static LPFNDLLNEWTESS           s_lpfnNewTess;
static LPFNDLLTESSBEGINCONTOUR  s_lpfnTessBeginContour;
static LPFNDLLTESSENDCONTOUR    s_lpfnTessEndContour;
static LPFNDLLTESSBEGINPOLYGON  s_lpfnTessBeginPolygon;
static LPFNDLLTESSENDPOLYGON    s_lpfnTessEndPolygon;
static LPFNDLLDELETETESS        s_lpfnDeleteTess;
static LPFNDLLTESSVERTEX        s_lpfnTessVertex;
static LPFNDLLTESSCALLBACK      s_lpfnTessCallback;

//  Mapping of pointers to glu functions by substitute macro
#define gluTessProperty         s_lpfnTessProperty
#define gluNewTess              s_lpfnNewTess
#define gluTessBeginContour     s_lpfnTessBeginContour
#define gluTessEndContour       s_lpfnTessEndContour
#define gluTessBeginPolygon     s_lpfnTessBeginPolygon
#define gluTessEndPolygon       s_lpfnTessEndPolygon
#define gluDeleteTess           s_lpfnDeleteTess
#define gluTessVertex           s_lpfnTessVertex
#define gluTessCallback         s_lpfnTessCallback


#endif
#endif
//  Flag to tell that dll is ready
bool           s_glu_dll_ready;
HINSTANCE      s_hGLU_DLL;                   // Handle to DLL
#endif



bool ispolysame(polyout *p1, polyout *p2)
{
    int i2;

    if(p1->nvert != p2->nvert)
        return false;

    int v10 = p1->vertex_index_list[0];

    for(i2 = 0 ; i2 < p2->nvert ; i2++)
    {
        if(p2->vertex_index_list[i2] == v10)
            break;
    }
    if(i2 == p2->nvert)
        return false;

    for(int j = 0 ; j<p1->nvert ; j++)
    {
        if(p1->vertex_index_list[j] != p2->vertex_index_list[i2])
            return false;
        i2++;
        if(i2 == p2->nvert)
            i2 = 0;
    }

    return true;
}


//------------------------------------------------------------------------------
//          Extended_Geometry Implementation
//------------------------------------------------------------------------------
Extended_Geometry::Extended_Geometry()
{
      vertex_array = NULL;
      contour_array = NULL;
}

Extended_Geometry::~Extended_Geometry()
{
      free(vertex_array);
      free(contour_array);
}


//------------------------------------------------------------------------------
//          PolyTessGeo Implementation
//------------------------------------------------------------------------------
PolyTessGeo63::PolyTessGeo63()
{
}

#if 0
//      Build PolyTessGeo Object from Extended_Geometry
PolyTessGeo::PolyTessGeo(Extended_Geometry *pxGeom)
{
      m_ppg_head = NULL;
      m_bOK = false;

      m_pxgeom = pxGeom;

}
#endif

#if 0
//      Build PolyTessGeo Object from OGR Polygon
PolyTessGeo::PolyTessGeo(OGRPolygon *poly, bool bSENC_SM, double ref_lat, double ref_lon, bool bUseInternalTess)
{
    ErrorCode = 0;
    m_ppg_head = NULL;
    m_pxgeom = NULL;

    m_ref_lat = ref_lat;
    m_ref_lon = ref_lon;

    if(bUseInternalTess){
        printf("internal tess\n");
        ErrorCode = PolyTessGeoTri(poly, bSENC_SM, ref_lat, ref_lon);
    }
    else {
#ifdef USE_GLU_TESS
printf("USE_GLU_TESS tess\n");
ErrorCode = PolyTessGeoGL(poly, bSENC_SM, ref_lat, ref_lon);
#else
printf("PolyTessGeoTri tess %d\n", s_idx++);
ErrorCode = PolyTessGeoTri(poly, bSENC_SM, ref_lat, ref_lon);
#endif
    }
}
#endif


//      Build PolyGeo Object from SENC file record
PolyTessGeo63::PolyTessGeo63(unsigned char *polybuf, int nrecl, int index,  int senc_file_version)
{
#define POLY_LINE_HDR_MAX 1000
//      Todo Add a try/catch set here, in case SENC file is corrupted??

    m_pxgeom = NULL;

    char hdr_buf[POLY_LINE_HDR_MAX];
    int twkb_len;

    m_buf_head = (char *) polybuf;                      // buffer beginning
    m_buf_ptr = m_buf_head;
    m_nrecl = nrecl;


    my_bufgets( hdr_buf, POLY_LINE_HDR_MAX );
    //  Read the s57obj extents as lat/lon
    sscanf(hdr_buf, "  POLYTESSGEOPROP %lf %lf %lf %lf",
           &xmin, &ymin, &xmax, &ymax);


    PolyTriGroup *ppg = new PolyTriGroup;
    ppg->m_bSMSENC = true;
    ppg->data_type = DATA_TYPE_DOUBLE;

    int nctr;
    my_bufgets( hdr_buf, POLY_LINE_HDR_MAX );
    sscanf(hdr_buf, "Contours/nWKB %d %d", &nctr, &twkb_len);
    ppg->nContours = nctr;
    ppg->pn_vertex = (int *)malloc(nctr * sizeof(int));
    int *pctr = ppg->pn_vertex;

    size_t buf_len = wxMax(twkb_len + 2, 20 + (nctr * 6));
    char *buf = (char *)malloc(buf_len);        // allocate a buffer guaranteed big enough

    my_bufgets( buf, buf_len );                 // contour nVert, as a char line

    wxString ivc_str(buf + 10,  wxConvUTF8);
    wxStringTokenizer tkc(ivc_str, wxT(" ,\n"));
    long icv = 0;

    while ( tkc.HasMoreTokens() )
    {
        wxString token = tkc.GetNextToken();
        if(token.ToLong(&icv))
        {
            if(icv)
            {
                *pctr = icv;
                pctr++;
            }
        }
    }


    //  Read Raw Geometry

    float *ppolygeo = (float *)malloc(twkb_len + 1);    // allow for crlf
    memmove(ppolygeo,  m_buf_ptr, twkb_len + 1);
    m_buf_ptr += twkb_len + 1;
    ppg->pgroup_geom = ppolygeo;



    TriPrim **p_prev_triprim = &(ppg->tri_prim_head);

    //  Read the PTG_Triangle Geometry in a loop
    unsigned int tri_type;
    int nvert;
    int nvert_max = 0;
    int total_byte_size = 0;
    bool not_finished = true;
    while(not_finished)
    {
        if((m_buf_ptr - m_buf_head) != m_nrecl)
        {
            int *pi = (int *)m_buf_ptr;
            tri_type = *pi++;
            nvert = *pi;
            m_buf_ptr += 2 * sizeof(int);

            //    Here is the usual stop condition, which results from
            //    interpreting the string "POLYEND" as an int
            if(tri_type == 0x594c4f50)
            {
                  not_finished = false;
                  break;
            }

            TriPrim *tp = new TriPrim;
            *p_prev_triprim = tp;                               // make the link
            p_prev_triprim = &(tp->p_next);
            tp->p_next = NULL;

            tp->type = tri_type;
            tp->nVert = nvert;

            if(nvert > nvert_max )                          // Keep a running tab of largest vertex count
                  nvert_max = nvert;

            if(senc_file_version > 122){
                int byte_size = nvert * 2 * sizeof(float);
                total_byte_size += byte_size;

                tp->p_vertex = (double *)malloc(byte_size);
                memmove(tp->p_vertex, m_buf_ptr, byte_size);
                m_buf_ptr += byte_size;
            }
            else{
                int byte_size = nvert * 2 * sizeof(double);
                total_byte_size += byte_size;

                tp->p_vertex = (double *)malloc(byte_size);
                memmove(tp->p_vertex, m_buf_ptr, byte_size);
                m_buf_ptr += byte_size;
            }

            //  Read the triangle primitive bounding box as lat/lon
            double *pbb = (double *)m_buf_ptr;

#ifdef ARMHF
            double abox[4];
            memcpy(&abox[0], pbb, 4 * sizeof(double));
            tp->minx = abox[0];
            tp->maxx = abox[1];
            tp->miny = abox[2];
            tp->maxy = abox[3];
#else
            tp->minx = *pbb++;
            tp->maxx = *pbb++;
            tp->miny = *pbb++;
            tp->maxy = *pbb;
#endif


            m_buf_ptr += 4 * sizeof(double);

        }
        else                                    // got end of poly
            not_finished = false;
    }                   // while

   //  Convert the vertex arrays into a single float memory allocation to enable efficient access later
    if(senc_file_version > 122){
        unsigned char *vbuf = (unsigned char *)malloc(total_byte_size);
        TriPrim *p_tp = ppg->tri_prim_head;
        unsigned char *p_run = vbuf;
        while( p_tp ) {
            memcpy(p_run, p_tp->p_vertex, p_tp->nVert * 2 * sizeof(float));
            free(p_tp->p_vertex);
            p_tp->p_vertex = (double  *)p_run;
            p_run += p_tp->nVert * 2 * sizeof(float);
            p_tp = p_tp->p_next; // pick up the next in chain
        }
        ppg->bsingle_alloc = true;
        ppg->single_buffer = vbuf;
        ppg->single_buffer_size = total_byte_size;
        ppg->data_type = DATA_TYPE_FLOAT;
    }


    m_ppg_head = ppg;
    m_nvertex_max = nvert_max;

    free(buf);

    ErrorCode = 0;
    m_bOK = true;

}


//      Build PolyTessGeo Object from OGR Polygon
//      Using internal Triangle tesselator
#if 0
int PolyTessGeo::PolyTessGeoTri(OGRPolygon *poly, bool bSENC_SM, double ref_lat, double ref_lon)
{
    //  Make a quick sanity check of the polygon coherence
    bool b_ok = true;
    OGRLineString *tls = poly->getExteriorRing();
    if(!tls) {
        b_ok = false;
    }
    else {
        int tnpta  = poly->getExteriorRing()->getNumPoints();
        if(tnpta < 3 )
            b_ok = false;
    }


    for( int iir=0 ; iir < poly->getNumInteriorRings() ; iir++)
    {
        int tnptr = poly->getInteriorRing(iir)->getNumPoints();
        if( tnptr < 3 )
            b_ok = false;
    }

    if( !b_ok )
        return ERROR_BAD_OGRPOLY;


    m_pxgeom = NULL;

    int iir, ip;

    tess_orient = TESS_HORZ;                    // prefer horizontal tristrips

//    PolyGeo BBox
    OGREnvelope Envelope;
    poly->getEnvelope(&Envelope);
    xmin = Envelope.MinX;
    ymin = Envelope.MinY;
    xmax = Envelope.MaxX;
    ymax = Envelope.MaxY;


//      Get total number of contours
    ncnt = 1;                         // always exterior ring
    int nint = poly->getNumInteriorRings();  // interior rings
    ncnt += nint;

//      Allocate cntr array
    int *cntr = (int *)malloc(ncnt * sizeof(int));

//      Get total number of points(vertices)
    int npta  = poly->getExteriorRing()->getNumPoints();
    npta += 2;                            // fluff

    for( iir=0 ; iir < nint ; iir++)
    {
        int nptr = poly->getInteriorRing(iir)->getNumPoints();
        npta += nptr + 2;
    }

    pt *geoPt = (pt*)calloc((npta + 1) * sizeof(pt), 1);     // vertex array

//      Create input structures

//    Exterior Ring
    int npte  = poly->getExteriorRing()->getNumPoints();
    cntr[0] = npte;

    pt *ppt = geoPt;
    ppt->x = 0.;
    ppt->y = 0.;
    ppt++;                                            // vertex 0 is unused

//  Check and account for winding direction of ring
    bool cw = !(poly->getExteriorRing()->isClockwise() == 0);

    double x0, y0, x, y;
    OGRPoint p;

    if(cw)
    {
        poly->getExteriorRing()->getPoint(0, &p);
        x0 = p.getX();
        y0 = p.getY();
    }
    else
    {
        poly->getExteriorRing()->getPoint(npte-1, &p);
        x0 = p.getX();
        y0 = p.getY();
    }


//  Transcribe points to vertex array, in proper order with no duplicates
    for(ip = 0 ; ip < npte ; ip++)
    {

        int pidx;
        if(cw)
            pidx = npte - ip - 1;

        else
            pidx = ip;

        poly->getExteriorRing()->getPoint(pidx, &p);
        x = p.getX();
        y = p.getY();

        if((fabs(x-x0) > EQUAL_EPS) || (fabs(y-y0) > EQUAL_EPS))
        {
            ppt->x = x;
            ppt->y = y;

            ppt++;
        }
        else
            cntr[0]--;

        x0 = x;
        y0 = y;
    }


//  Now the interior contours
    for(iir=0 ; iir < nint ; iir++)
    {
        int npti = poly->getInteriorRing(iir)->getNumPoints();
        cntr[iir + 1] = npti;


      //  Check and account for winding direction of ring
        bool cw = !(poly->getInteriorRing(iir)->isClockwise() == 0);

        if(!cw)
        {
            poly->getInteriorRing(iir)->getPoint(0, &p);
            x0 = p.getX();
            y0 = p.getY();
        }
        else
        {
            poly->getInteriorRing(iir)->getPoint(npti-1, &p);
            x0 = p.getX();
            y0 = p.getY();
        }

//  Transcribe points to vertex array, in proper order with no duplicates
        for(int ip = 0 ; ip < npti ; ip++)
        {
            OGRPoint p;
            int pidx;
            if(!cw)                               // interior contours must be cw
                pidx = npti - ip - 1;
            else
                pidx = ip;

            poly->getInteriorRing(iir)->getPoint(pidx, &p);
            x = p.getX();
            y = p.getY();

            if((fabs(x-x0) > EQUAL_EPS) || (fabs(y-y0) > EQUAL_EPS))
            {
                ppt->x = x;
                ppt->y = y;
                ppt++;
            }
            else
                cntr[iir+1]--;

            x0 = x;
            y0 = y;

        }
    }

    polyout *polys = triangulate_polygon(ncnt, cntr, (double (*)[2])geoPt);


//  Check the triangles
    //  Especially looking for poorly formed polys
    //  These may come from several sources, all
    //  of which should be considered latent bugs in the trapezator.

    //  Known to occur:
    //  Trapezator fails if any two inner contours share a common vertex.
    //  Found on US5VA19M.000

    polyout *pck = polys;
    while(NULL != pck)
    {
        if(pck->is_valid)
        {
            int *ivs = pck->vertex_index_list;

            for(int i3 = 0 ; i3 < pck->nvert-1 ; i3++)
            {
                int ptest = ivs[i3];
                for(int i4=i3+1 ; i4 < pck->nvert ; i4++)
                {
                    if(ptest == ivs[i4])
                    {
                        pck->is_valid = false;
                    }
                }
            }
        }

        pck = (polyout *)pck->poly_next;
    }


//  Walk the list once to get poly count
    polyout *pr;
    pr = polys;
    int npoly0 = 0;
    while(NULL != pr)
    {
        pr = (polyout *)pr->poly_next;
        npoly0++;
    }

//  Check the list for duplicates

    pr = polys;
    for(int idt = 0 ; idt<npoly0-1 ; idt++)
    {
        polyout *p1 = pr;

        polyout *p2 = (polyout *)pr->poly_next;
        while(NULL != p2)
        {
            if(p1->is_valid && p2->is_valid)
            {
                if(ispolysame(p1, p2))
                    p1->is_valid = false;
            }
            p2 = (polyout *)p2->poly_next;
        }

        pr = (polyout *)pr->poly_next;
    }

//  Walk the list again to get unique poly count
    pr = polys;
    int npoly = 0;
    while(NULL != pr)
    {
        if(pr->is_valid)
            npoly++;
        pr = (polyout *)pr->poly_next;

    }



    //  Create the data structuresSRC_S63

    m_nvertex_max = 0;

    m_ppg_head = new PolyTriGroup;
    m_ppg_head->m_bSMSENC = s_bSENC_SM;

    m_ppg_head->nContours = ncnt;

    m_ppg_head->pn_vertex = cntr;             // pointer to array of poly vertex counts


//  Transcribe the raw geometry buffer
//  Converting to float as we go, and
//  allowing for tess_orient

    nwkb = (npta +1) * 2 * sizeof(float);
    m_ppg_head->pgroup_geom = (float *)malloc(nwkb);
    float *vro = m_ppg_head->pgroup_geom;
    float tx,ty;

    for(ip = 1 ; ip < npta + 1 ; ip++)
    {
        if(TESS_HORZ == tess_orient)
        {
            ty = geoPt[ip].y;
            tx = geoPt[ip].x;
        }
        else
        {
            tx = geoPt[ip].x;
            ty = geoPt[ip].y;
        }

        if(bSENC_SM)
        {
            //  Calculate SM from chart common reference point
            double easting, northing;
            toSM(ty, tx, ref_lat, ref_lon, &easting, &northing);
            *vro++ = easting;              // x
            *vro++ = northing;             // y

            printf(" %d %g %g %g %g %g %g\n", npta,ref_lat, ref_lon, easting, northing, ty, tx);
        }
        else
        {
            *vro++ = tx;                  // lon
            *vro++ = ty;                  // lat
        }

    }



       //  Now the Triangle Primitives

    TriPrim *pTP = NULL;
    TriPrim *pTP_Head = NULL;
    TriPrim *pTP_Last = NULL;

    pr = polys;
    while(NULL != pr)
    {
        if(pr->is_valid)
        {
            pTP = new TriPrim;
            if(NULL == pTP_Last)
            {
                pTP_Head = pTP;
                pTP_Last = pTP;
            }
            else
            {
                pTP_Last->p_next = pTP;
                pTP_Last = pTP;
            }

            pTP->p_next = NULL;
            pTP->type = PTG_TRIANGLES;
            pTP->nVert = pr->nvert;

            if(pr->nvert > m_nvertex_max)
                  m_nvertex_max = pr->nvert;                         // keep track of largest vertex count

            //  Convert to SM
            pTP->p_vertex = (double *)malloc(pr->nvert * 2 * sizeof(double));
            double *pdd = pTP->p_vertex;
            int *ivr = pr->vertex_index_list;
            if(bSENC_SM)
            {
                for(int i=0 ; i<pr->nvert ; i++)
                {
                    int ivp = ivr[i];
                    double dlon = geoPt[ivp].x;
                    double dlat = geoPt[ivp].y;

                    double easting, northing;
                    toSM(dlat, dlon, ref_lat, ref_lon, &easting, &northing);
                    *pdd++ = easting;
                    *pdd++ = northing;
                }
            }

            else
            {
                for(int i=0 ; i<pr->nvert ; i++)
                {
                    int ivp = ivr[i];

                    memcpy(pdd++, &geoPt[ivp].x, sizeof(double));
                    memcpy(pdd++, &geoPt[ivp].y, sizeof(double));
                }
            }
            //  Calculate bounding box as lat/lon

            pTP->p_bbox = new wxBoundingBox;

            float sxmax = -179;                   // this poly BBox
            float sxmin = 170;
            float symax = -90;
            float symin = 90;

            for(int iv=0 ; iv < pr->nvert ; iv++)
            {
                int *ivr = pr->vertex_index_list;
                int ivp = ivr[iv];
                double xd = geoPt[ivp].x;
                double yd = geoPt[ivp].y;

                sxmax = wxMax(xd, sxmax);
                sxmin = wxMin(xd, sxmin);
                symax = wxMax(yd, symax);
                symin = wxMin(yd, symin);
            }

            pTP->p_bbox->SetMin(sxmin, symin);
            pTP->p_bbox->SetMax(sxmax, symax);

        }
        pr = (polyout *)pr->poly_next;
    }

    m_ppg_head->tri_prim_head = pTP_Head;         // head of linked list of TriPrims


//  Free the polyout structure
    pr = polys;
    while(NULL != pr)
    {
        free(pr->vertex_index_list);

        polyout *pf = pr;
        pr = (polyout *)pr->poly_next;
        free(pf);
    }

    free (geoPt);

    m_bOK = true;

    return 0;
}

int PolyTessGeo::Write_PolyTriGroup( FILE *ofs)
{
    wxString    sout;
    wxString    sout1;
    wxString    stemp;

    PolyTriGroup *pPTG = m_ppg_head;


//  Begin creating the output record
//      Use a wxMemoryStream for temporary record output.
//      When all finished, we'll touch up a few items before
//      committing to disk.


    ostream1 = new wxMemoryOutputStream(NULL, 0);                      // auto buffer creation
    ostream2 = new wxMemoryOutputStream(NULL, 0);                      // auto buffer creation

//  Create initial known part of the output record


    stemp.sprintf( _T("  POLYTESSGEOPROP %f %f %f %f\n"),
                   xmin, ymin, xmax, ymax);            // PolyTessGeo Properties
    sout += stemp;

//  Transcribe the true number of  contours, and the raw geometry wkb size
    stemp.sprintf( _T("Contours/nWKB %d %d\n"),  ncnt, nwkb);
    sout += stemp;


//  Transcribe the contour counts
    stemp.sprintf(_T("Contour nV"));
    sout += stemp;
    for(int i=0 ; i<ncnt ; i++)
    {
        stemp.sprintf( _T(" %d"), pPTG->pn_vertex[i]);
        sout += stemp;
    }
    stemp.sprintf( _T("\n"));
    sout += stemp;
    ostream1->Write(sout.mb_str(), sout.Len());

//  Transcribe the raw geometry buffer
    ostream1->Write(pPTG->pgroup_geom,nwkb);
    stemp.sprintf( _T("\n"));
    ostream1->Write(stemp.mb_str(), stemp.Len());


//  Transcribe the TriPrim chain

    TriPrim *pTP = pPTG->tri_prim_head;         // head of linked list of TriPrims


    while(pTP)
    {
        ostream2->Write(&pTP->type, sizeof(int));
        ostream2->Write(&pTP->nVert, sizeof(int));

        ostream2->Write( pTP->p_vertex, pTP->nVert * 2 * sizeof(double));

        //  Write out the object bounding box as lat/lon
        double minx = pTP->p_bbox->GetMinX();
        double maxx = pTP->p_bbox->GetMaxX();
        double miny = pTP->p_bbox->GetMinY();
        double maxy = pTP->p_bbox->GetMaxY();
        ostream2->Write(&minx, sizeof(double));
        ostream2->Write(&maxx, sizeof(double));
        ostream2->Write(&miny, sizeof(double));
        ostream2->Write(&maxy, sizeof(double));


        pTP = pTP->p_next;
    }


    stemp.sprintf( _T("POLYEND\n"));
    ostream2->Write(stemp.mb_str(), stemp.Len());

    int nrecl = ostream1->GetSize() + ostream2->GetSize();
    stemp.sprintf( _T("  POLYTESSGEO  %08d %g %g\n"), nrecl, m_ref_lat, m_ref_lon);

    fwrite(stemp.mb_str(), 1, stemp.Len(), ofs);                 // Header, + record length

    char *tb = (char *)malloc(ostream1->GetSize());
    ostream1->CopyTo(tb, ostream1->GetSize());
    fwrite(tb, 1, ostream1->GetSize(), ofs);
    free(tb);

    tb = (char *)malloc(ostream2->GetSize());
    ostream2->CopyTo(tb, ostream2->GetSize());
    fwrite(tb, 1, ostream2->GetSize(), ofs);
    free(tb);

    delete ostream1;
    delete ostream2;

    return 0;
}

int PolyTessGeo::Write_PolyTriGroup( wxOutputStream &out_stream)
{
      wxString    sout;
      wxString    sout1;
      wxString    stemp;

      PolyTriGroup *pPTG = m_ppg_head;


//  Begin creating the output record
//      Use a wxMemoryStream for temporary record output.
//      When all finished, we'll touch up a few items before
//      committing to disk.


      ostream1 = new wxMemoryOutputStream(NULL, 0);                      // auto buffer creation
      ostream2 = new wxMemoryOutputStream(NULL, 0);                      // auto buffer creation

//  Create initial known part of the output record


      stemp.sprintf( _T("  POLYTESSGEOPROP %f %f %f %f\n"),
                     xmin, ymin, xmax, ymax);            // PolyTessGeo Properties
      sout += stemp;

//  Transcribe the true number of  contours, and the raw geometry wkb size
      stemp.sprintf( _T("Contours/nWKB %d %d\n"),  ncnt, nwkb);
      sout += stemp;


//  Transcribe the contour counts
      stemp.sprintf(_T("Contour nV"));
      sout += stemp;
      for(int i=0 ; i<ncnt ; i++)
      {
            stemp.sprintf( _T(" %d"), pPTG->pn_vertex[i]);
            sout += stemp;
      }
      stemp.sprintf( _T("\n"));
      sout += stemp;
      ostream1->Write(sout.mb_str(), sout.Len());

//  Transcribe the raw geometry buffer
      ostream1->Write(pPTG->pgroup_geom,nwkb);
      stemp.sprintf( _T("\n"));
      ostream1->Write(stemp.mb_str(), stemp.Len());


//  Transcribe the TriPrim chain

      TriPrim *pTP = pPTG->tri_prim_head;         // head of linked list of TriPrims


      while(pTP)
      {
            ostream2->Write(&pTP->type, sizeof(int));
            ostream2->Write(&pTP->nVert, sizeof(int));

            ostream2->Write( pTP->p_vertex, pTP->nVert * 2 * sizeof(double));

        //  Write out the object bounding box as lat/lon
            double minx = pTP->p_bbox->GetMinX();
            double maxx = pTP->p_bbox->GetMaxX();
            double miny = pTP->p_bbox->GetMinY();
            double maxy = pTP->p_bbox->GetMaxY();
            ostream2->Write(&minx, sizeof(double));
            ostream2->Write(&maxx, sizeof(double));
            ostream2->Write(&miny, sizeof(double));
            ostream2->Write(&maxy, sizeof(double));


            pTP = pTP->p_next;
      }


      stemp.sprintf( _T("POLYEND\n"));
      ostream2->Write(stemp.mb_str(), stemp.Len());

      int nrecl = ostream1->GetSize() + ostream2->GetSize();
      stemp.sprintf( _T("  POLYTESSGEO  %08d %g %g\n"), nrecl, m_ref_lat, m_ref_lon);

      out_stream.Write(stemp.mb_str(), stemp.Len());                 // Header, + record length

      char *tb = (char *)malloc(ostream1->GetSize());
      ostream1->CopyTo(tb, ostream1->GetSize());

      out_stream.Write(tb, ostream1->GetSize());
      free(tb);

      tb = (char *)malloc(ostream2->GetSize());
      ostream2->CopyTo(tb, ostream2->GetSize());
      out_stream.Write(tb, ostream2->GetSize());

      free(tb);


      delete ostream1;
      delete ostream2;

      return 0;
}


#endif

int PolyTessGeo63::my_bufgets( char *buf, int buf_len_max )
{
    char        chNext;
    int         nLineLen = 0;
    char        *lbuf;

    lbuf = buf;


    while( (nLineLen < buf_len_max) &&((m_buf_ptr - m_buf_head) < m_nrecl) )
    {
        chNext = *m_buf_ptr++;

        /* each CR/LF (or LF/CR) as if just "CR" */
        if( chNext == 10 || chNext == 13 )
        {
            chNext = '\n';
        }

        *lbuf = chNext; lbuf++, nLineLen++;

        if( chNext == '\n' )
        {
            *lbuf = '\0';
            return nLineLen;
        }
    }

    *(lbuf) = '\0';
    return nLineLen;
}



PolyTessGeo63::~PolyTessGeo63()
{

    delete  m_ppg_head;

    delete  m_pxgeom;

#ifdef USE_GLU_TESS
    if(s_pwork_buf)
        free( s_pwork_buf );
    s_pwork_buf = NULL;
#endif

}

int PolyTessGeo63::BuildDeferredTess(void)
{
#ifdef USE_GLU_TESS
    return BuildTessGL();
#else
    return 0;
#endif
}


// void PolyTessGeo::GetRefPos( double *lat, double *lon)
// {
//     if(lat)
//         *lat = m_ref_lat;
//     if(lon)
//         *lon = m_ref_lon;
// }


#ifdef USE_GLU_TESS


#ifdef __WXMSW__
#define __CALL_CONVENTION __stdcall
#else
#define __CALL_CONVENTION
#endif


void __CALL_CONVENTION beginCallback(GLenum which);
void __CALL_CONVENTION errorCallback(GLenum errorCode);
void __CALL_CONVENTION endCallback(void);
void __CALL_CONVENTION vertexCallback(GLvoid *vertex);
void __CALL_CONVENTION combineCallback(GLdouble coords[3],
                     GLdouble *vertex_data[4],
                     GLfloat weight[4], GLdouble **dataOut );


//      Build PolyTessGeo Object from OGR Polygon
//      Using OpenGL/GLU tesselator
int PolyTessGeo63::PolyTessGeoGL(OGRPolygon *poly, bool bSENC_SM, double ref_lat, double ref_lon)
{
#ifdef ocpnUSE_GL

    int iir, ip;
    int *cntr;
    GLdouble *geoPt;

    wxString    sout;
    wxString    sout1;
    wxString    stemp;

    //  Make a quick sanity check of the polygon coherence
    bool b_ok = true;
    OGRLineString *tls = poly->getExteriorRing();
    if(!tls) {
        b_ok = false;
    }
    else {
        int tnpta  = poly->getExteriorRing()->getNumPoints();
        if(tnpta < 3 )
            b_ok = false;
    }


    for( iir=0 ; iir < poly->getNumInteriorRings() ; iir++)
    {
        int tnptr = poly->getInteriorRing(iir)->getNumPoints();
        if( tnptr < 3 )
            b_ok = false;
    }

    if( !b_ok )
        return ERROR_BAD_OGRPOLY;


#ifdef __WXMSW__
//  If using the OpenGL dlls provided with Windows,
//  load the dll and establish addresses of the entry points needed

#ifdef USE_GLU_DLL

    if(!s_glu_dll_ready)
    {


        s_hGLU_DLL = LoadLibrary("glu32.dll");
        if (s_hGLU_DLL != NULL)
        {
            s_lpfnTessProperty = (LPFNDLLTESSPROPERTY)GetProcAddress(s_hGLU_DLL,"gluTessProperty");
            s_lpfnNewTess = (LPFNDLLNEWTESS)GetProcAddress(s_hGLU_DLL, "gluNewTess");
            s_lpfnTessBeginContour = (LPFNDLLTESSBEGINCONTOUR)GetProcAddress(s_hGLU_DLL, "gluTessBeginContour");
            s_lpfnTessEndContour = (LPFNDLLTESSENDCONTOUR)GetProcAddress(s_hGLU_DLL, "gluTessEndContour");
            s_lpfnTessBeginPolygon = (LPFNDLLTESSBEGINPOLYGON)GetProcAddress(s_hGLU_DLL, "gluTessBeginPolygon");
            s_lpfnTessEndPolygon = (LPFNDLLTESSENDPOLYGON)GetProcAddress(s_hGLU_DLL, "gluTessEndPolygon");
            s_lpfnDeleteTess = (LPFNDLLDELETETESS)GetProcAddress(s_hGLU_DLL, "gluDeleteTess");
            s_lpfnTessVertex = (LPFNDLLTESSVERTEX)GetProcAddress(s_hGLU_DLL, "gluTessVertex");
            s_lpfnTessCallback = (LPFNDLLTESSCALLBACK)GetProcAddress(s_hGLU_DLL, "gluTessCallback");

            s_glu_dll_ready = true;
        }
        else
        {
            return ERROR_NO_DLL;
        }
    }

#endif
#endif


    //  Allocate a work buffer, which will be grown as needed
#define NINIT_BUFFER_LEN 10000
    s_pwork_buf = (GLdouble *)malloc(NINIT_BUFFER_LEN * 2 * sizeof(GLdouble));
    s_buf_len = NINIT_BUFFER_LEN * 2;
    s_buf_idx = 0;

      //    Create an array to hold pointers to allocated vertices created by "combine" callback,
      //    so that they may be deleted after tesselation.
    s_pCombineVertexArray = new wxArrayPtrVoid;

    //  Create tesselator
    GLUtessobj = gluNewTess();

    //  Register the callbacks
    gluTessCallback(GLUtessobj, GLU_TESS_BEGIN,   (GLvoid (__CALL_CONVENTION *) ())&beginCallback);
    gluTessCallback(GLUtessobj, GLU_TESS_BEGIN,   (GLvoid (__CALL_CONVENTION *) ())&beginCallback);
    gluTessCallback(GLUtessobj, GLU_TESS_VERTEX,  (GLvoid (__CALL_CONVENTION *) ())&vertexCallback);
    gluTessCallback(GLUtessobj, GLU_TESS_END,     (GLvoid (__CALL_CONVENTION *) ())&endCallback);
    gluTessCallback(GLUtessobj, GLU_TESS_COMBINE, (GLvoid (__CALL_CONVENTION *) ())&combineCallback);

//    gluTessCallback(GLUtessobj, GLU_TESS_ERROR,   (GLvoid (__CALL_CONVENTION *) ())&errorCallback);

//    glShadeModel(GL_SMOOTH);
    gluTessProperty(GLUtessobj, GLU_TESS_WINDING_RULE,
                    GLU_TESS_WINDING_POSITIVE );

    //  gluTess algorithm internally selects vertically oriented triangle strips and fans.
    //  This orientation is not optimal for conventional memory-mapped raster display shape filling.
    //  We can "fool" the algorithm by interchanging the x and y vertex values passed to gluTessVertex
    //  and then reverting x and y on the resulting vertexCallbacks.
    //  In this implementation, we will explicitely set the preferred orientation.

    //Set the preferred orientation
    tess_orient = TESS_HORZ;                    // prefer horizontal tristrips



//    PolyGeo BBox as lat/lon
    OGREnvelope Envelope;
    poly->getEnvelope(&Envelope);
    xmin = Envelope.MinX;
    ymin = Envelope.MinY;
    xmax = Envelope.MaxX;
    ymax = Envelope.MaxY;


//      Get total number of contours
    ncnt = 1;                         // always exterior ring
    int nint = poly->getNumInteriorRings();  // interior rings
    ncnt += nint;


//      Allocate cntr array
    cntr = (int *)malloc(ncnt * sizeof(int));


//      Get total number of points(vertices)
    int npta  = poly->getExteriorRing()->getNumPoints();
    cntr[0] = npta;
    npta += 2;                            // fluff

    for( iir=0 ; iir < nint ; iir++)
    {
        int nptr = poly->getInteriorRing(iir)->getNumPoints();
        cntr[iir+1] = nptr;

        npta += nptr + 2;
    }

//    printf("pPoly npta: %d\n", npta);

    geoPt = (GLdouble *)malloc((npta) * 3 * sizeof(GLdouble));     // vertex array



   //  Grow the work buffer if necessary

    if((npta * 4) > s_buf_len)
    {
        s_pwork_buf = (GLdouble *)realloc(s_pwork_buf, npta * 4 * 2 * sizeof(GLdouble *));
        s_buf_len = npta * 4 * 2;
    }


//  Define the polygon
    gluTessBeginPolygon(GLUtessobj, NULL);


//      Create input structures

//    Exterior Ring
    int npte  = poly->getExteriorRing()->getNumPoints();
    cntr[0] = npte;

    GLdouble *ppt = geoPt;


//  Check and account for winding direction of ring
    bool cw = !(poly->getExteriorRing()->isClockwise() == 0);

    double x0, y0, x, y;
    OGRPoint p;

    if(cw)
    {
        poly->getExteriorRing()->getPoint(0, &p);
        x0 = p.getX();
        y0 = p.getY();
    }
    else
    {
        poly->getExteriorRing()->getPoint(npte-1, &p);
        x0 = p.getX();
        y0 = p.getY();
    }


    gluTessBeginContour(GLUtessobj);

//  Transcribe points to vertex array, in proper order with no duplicates
//   also, accounting for tess_orient
    for(ip = 0 ; ip < npte ; ip++)
    {
        int pidx;
        if(cw)
            pidx = npte - ip - 1;

        else
            pidx = ip;

        poly->getExteriorRing()->getPoint(pidx, &p);
        x = p.getX();
        y = p.getY();

        if((fabs(x-x0) > EQUAL_EPS) || (fabs(y-y0) > EQUAL_EPS))
        {
            GLdouble *ppt_temp = ppt;
            if(tess_orient == TESS_VERT)
            {
                *ppt++ = x;
                *ppt++ = y;
            }
            else
            {
                *ppt++ = y;
                *ppt++ = x;
            }

            *ppt++ = 0.0;

            gluTessVertex( GLUtessobj, ppt_temp, ppt_temp ) ;
  //printf("tess from pPoly, external vertex %g %g\n", x, y);


        }
        else
            cntr[0]--;

        x0 = x;
        y0 = y;
    }

    gluTessEndContour(GLUtessobj);




//  Now the interior contours
    for(iir=0 ; iir < nint ; iir++)
    {
        gluTessBeginContour(GLUtessobj);

        int npti = poly->getInteriorRing(iir)->getNumPoints();

      //  Check and account for winding direction of ring
        bool cw = !(poly->getInteriorRing(iir)->isClockwise() == 0);

        if(!cw)
        {
            poly->getInteriorRing(iir)->getPoint(0, &p);
            x0 = p.getX();
            y0 = p.getY();
        }
        else
        {
            poly->getInteriorRing(iir)->getPoint(npti-1, &p);
            x0 = p.getX();
            y0 = p.getY();
        }

//  Transcribe points to vertex array, in proper order with no duplicates
//   also, accounting for tess_orient
        for(int ip = 0 ; ip < npti ; ip++)
        {
            OGRPoint p;
            int pidx;
            if(!cw)                               // interior contours must be cw
                pidx = npti - ip - 1;
            else
                pidx = ip;

            poly->getInteriorRing(iir)->getPoint(pidx, &p);
            x = p.getX();
            y = p.getY();

            if((fabs(x-x0) > EQUAL_EPS) || (fabs(y-y0) > EQUAL_EPS))
            {
                GLdouble *ppt_temp = ppt;
                if(tess_orient == TESS_VERT)
                {
                    *ppt++ = x;
                    *ppt++ = y;
                }
                else
                {
                    *ppt++ = y;
                    *ppt++ = x;
                }
                *ppt++ = 0.0;

                gluTessVertex( GLUtessobj, ppt_temp, ppt_temp ) ;

//       printf("tess from Poly, internal vertex %d %g %g\n", ip, x, y);

            }
            else
                cntr[iir+1]--;

            x0 = x;
            y0 = y;

        }

        gluTessEndContour(GLUtessobj);
    }

    //  Store some SM conversion data in static store,
    //  for callback access
    s_ref_lat = ref_lat;
    s_ref_lon = ref_lon;
    s_bSENC_SM = bSENC_SM;

    s_bmerc_transform = false;

    //      Ready to kick off the tesselator

    s_pTPG_Last = NULL;
    s_pTPG_Head = NULL;

    s_nvmax = 0;

    gluTessEndPolygon(GLUtessobj);          // here it goes

    m_nvertex_max = s_nvmax;               // record largest vertex count, updates in callback


    //  Tesselation all done, so...

    //  Create the data structures

    m_ppg_head = new PolyTriGroup;
    m_ppg_head->m_bSMSENC = s_bSENC_SM;

    m_ppg_head->nContours = ncnt;

    m_ppg_head->pn_vertex = cntr;             // pointer to array of poly vertex counts
    m_ppg_head->data_type = DATA_TYPE_DOUBLE;


//  Transcribe the raw geometry buffer
//  Converting to float as we go, and
//  allowing for tess_orient
//  Also, convert to SM if requested

    nwkb = (npta +1) * 2 * sizeof(float);
    m_ppg_head->pgroup_geom = (float *)malloc(nwkb);
    float *vro = m_ppg_head->pgroup_geom;
    ppt = geoPt;
    float tx,ty;

    for(ip = 0 ; ip < npta ; ip++)
    {
        if(TESS_HORZ == tess_orient)
        {
            ty = *ppt++;
            tx = *ppt++;
        }
        else
        {
            tx = *ppt++;
            ty = *ppt++;
        }

        if(bSENC_SM)
        {
            //  Calculate SM from chart common reference point
            double easting, northing;
            toSM(ty, tx, ref_lat, ref_lon, &easting, &northing);
            *vro++ = easting;              // x
            *vro++ = northing;             // y
        }
        else
        {
            *vro++ = tx;                  // lon
            *vro++ = ty;                  // lat
        }

        ppt++;                      // skip z
    }

    m_ppg_head->tri_prim_head = s_pTPG_Head;         // head of linked list of TriPrims

    //  Convert the Triangle vertex arrays into a single memory allocation of floats
    //  to reduce SENC size and enable efficient access later

    //  First calculate the total byte size
    int total_byte_size = 0;
    TriPrim *p_tp = m_ppg_head->tri_prim_head;
    while( p_tp ) {
        total_byte_size += p_tp->nVert * 2 * sizeof(float);
        p_tp = p_tp->p_next; // pick up the next in chain
    }

    float *vbuf = (float *)malloc(total_byte_size);
    p_tp = m_ppg_head->tri_prim_head;
    float *p_run = vbuf;
    while( p_tp ) {
        float *pfbuf = p_run;
        for( int i=0 ; i < p_tp->nVert * 2 ; ++i){
            float x = (float)(p_tp->p_vertex[i]);
            *p_run++ = x;
        }

        free(p_tp->p_vertex);
        p_tp->p_vertex = (double *)pfbuf;
        p_tp = p_tp->p_next; // pick up the next in chain
    }
    m_ppg_head->bsingle_alloc = true;
    m_ppg_head->single_buffer = (unsigned char *)vbuf;
    m_ppg_head->single_buffer_size = total_byte_size;
    m_ppg_head->data_type = DATA_TYPE_FLOAT;


    gluDeleteTess(GLUtessobj);

    free( s_pwork_buf );
    s_pwork_buf = NULL;

    free (geoPt);

    //      Free up any "Combine" vertices created
    for(unsigned int i = 0; i < s_pCombineVertexArray->GetCount() ; i++)
          free (s_pCombineVertexArray->Item(i));
    delete s_pCombineVertexArray;

    m_bOK = true;

#endif          //    #ifdef ocpnUSE_GL

    return 0;
}

int PolyTessGeo::BuildTessGL(void)
{
#ifdef ocpnUSE_GL

      int iir, ip;
      int *cntr;
      GLdouble *geoPt;

      wxString    sout;
      wxString    sout1;
      wxString    stemp;


#ifdef __WXMSW__
//  If using the OpenGL dlls provided with Windows,
//  load the dll and establish addresses of the entry points needed

#ifdef USE_GLU_DLL

      if(!s_glu_dll_ready)
      {


            s_hGLU_DLL = LoadLibrary("glu32.dll");
            if (s_hGLU_DLL != NULL)
            {
                  s_lpfnTessProperty = (LPFNDLLTESSPROPERTY)GetProcAddress(s_hGLU_DLL,"gluTessProperty");
                  s_lpfnNewTess = (LPFNDLLNEWTESS)GetProcAddress(s_hGLU_DLL, "gluNewTess");
                  s_lpfnTessBeginContour = (LPFNDLLTESSBEGINCONTOUR)GetProcAddress(s_hGLU_DLL, "gluTessBeginContour");
                  s_lpfnTessEndContour = (LPFNDLLTESSENDCONTOUR)GetProcAddress(s_hGLU_DLL, "gluTessEndContour");
                  s_lpfnTessBeginPolygon = (LPFNDLLTESSBEGINPOLYGON)GetProcAddress(s_hGLU_DLL, "gluTessBeginPolygon");
                  s_lpfnTessEndPolygon = (LPFNDLLTESSENDPOLYGON)GetProcAddress(s_hGLU_DLL, "gluTessEndPolygon");
                  s_lpfnDeleteTess = (LPFNDLLDELETETESS)GetProcAddress(s_hGLU_DLL, "gluDeleteTess");
                  s_lpfnTessVertex = (LPFNDLLTESSVERTEX)GetProcAddress(s_hGLU_DLL, "gluTessVertex");
                  s_lpfnTessCallback = (LPFNDLLTESSCALLBACK)GetProcAddress(s_hGLU_DLL, "gluTessCallback");

                  s_glu_dll_ready = true;
            }
            else
            {
                  return ERROR_NO_DLL;
            }
      }

#endif
#endif


    //  Allocate a work buffer, which will be grown as needed
#define NINIT_BUFFER_LEN 10000
      s_pwork_buf = (GLdouble *)malloc(NINIT_BUFFER_LEN * 2 * sizeof(GLdouble));
      s_buf_len = NINIT_BUFFER_LEN * 2;
      s_buf_idx = 0;

      //    Create an array to hold pointers to allocated vertices created by "combine" callback,
      //    so that they may be deleted after tesselation.
      s_pCombineVertexArray = new wxArrayPtrVoid;

    //  Create tesselator
      GLUtessobj = gluNewTess();

    //  Register the callbacks
      gluTessCallback(GLUtessobj, GLU_TESS_BEGIN,   (GLvoid (__CALL_CONVENTION *) ())&beginCallback);
      gluTessCallback(GLUtessobj, GLU_TESS_BEGIN,   (GLvoid (__CALL_CONVENTION *) ())&beginCallback);
      gluTessCallback(GLUtessobj, GLU_TESS_VERTEX,  (GLvoid (__CALL_CONVENTION *) ())&vertexCallback);
      gluTessCallback(GLUtessobj, GLU_TESS_END,     (GLvoid (__CALL_CONVENTION *) ())&endCallback);
      gluTessCallback(GLUtessobj, GLU_TESS_COMBINE, (GLvoid (__CALL_CONVENTION *) ())&combineCallback);

//    gluTessCallback(GLUtessobj, GLU_TESS_ERROR,   (GLvoid (__CALL_CONVENTION *) ())&errorCallback);

//    glShadeModel(GL_SMOOTH);
      gluTessProperty(GLUtessobj, GLU_TESS_WINDING_RULE,
                      GLU_TESS_WINDING_POSITIVE );

    //  gluTess algorithm internally selects vertically oriented triangle strips and fans.
    //  This orientation is not optimal for conventional memory-mapped raster display shape filling.
    //  We can "fool" the algorithm by interchanging the x and y vertex values passed to gluTessVertex
    //  and then reverting x and y on the resulting vertexCallbacks.
    //  In this implementation, we will explicitely set the preferred orientation.

    //Set the preferred orientation
      tess_orient = TESS_HORZ;                    // prefer horizontal tristrips



//      Get total number of contours
      ncnt  = m_pxgeom->n_contours;

//      Allocate cntr array
      cntr = (int *)malloc(ncnt * sizeof(int));

//      Get total number of points(vertices)
      int npta  = m_pxgeom->contour_array[0];
      cntr[0] = npta;
      npta += 2;                            // fluff

      for( iir=0 ; iir < ncnt-1 ; iir++)
      {
            int nptr = m_pxgeom->contour_array[iir+1];
            cntr[iir+1] = nptr;

            npta += nptr + 2;             // fluff
      }



//      printf("xgeom npta: %d\n", npta);
      geoPt = (GLdouble *)malloc((npta) * 3 * sizeof(GLdouble));     // vertex array



   //  Grow the work buffer if necessary

      if((npta * 4) > s_buf_len)
      {
            s_pwork_buf = (GLdouble *)realloc(s_pwork_buf, npta * 4 * 2 * sizeof(GLdouble *));
            s_buf_len = npta * 4 * 2;
      }


//  Define the polygon
      gluTessBeginPolygon(GLUtessobj, NULL);


//      Create input structures

//    Exterior Ring
      int npte = m_pxgeom->contour_array[0];
      cntr[0] = npte;

      GLdouble *ppt = geoPt;


//  Check and account for winding direction of ring
      bool cw = true;

      double x0, y0, x, y;
      OGRPoint p;

      wxPoint2DDouble *pp = m_pxgeom->vertex_array;
      pp++;       // skip 0?

      if(cw)
      {
//            poly->getExteriorRing()->getPoint(0, &p);
            x0 = pp->m_x;
            y0 = pp->m_y;
      }
      else
      {
//            poly->getExteriorRing()->getPoint(npte-1, &p);
            x0 = pp[npte-1].m_x;
            y0 = pp[npte-1].m_y;
//            x0 = p.getX();
//            y0 = p.getY();
      }

//      pp++;

      gluTessBeginContour(GLUtessobj);

//  Transcribe points to vertex array, in proper order with no duplicates
//   also, accounting for tess_orient
      for(ip = 0 ; ip < npte ; ip++)
      {
            int pidx;
            if(cw)
                  pidx = npte - ip - 1;

            else
                  pidx = ip;

//            poly->getExteriorRing()->getPoint(pidx, &p);
            x = pp[pidx].m_x;
            y = pp[pidx].m_y;

//            pp++;

            if((fabs(x-x0) > EQUAL_EPS) || (fabs(y-y0) > EQUAL_EPS))
            {
                  GLdouble *ppt_temp = ppt;
                  if(tess_orient == TESS_VERT)
                  {
                        *ppt++ = x;
                        *ppt++ = y;
                  }
                  else
                  {
                        *ppt++ = y;
                        *ppt++ = x;
                  }

                  *ppt++ = 0.0;

                  gluTessVertex( GLUtessobj, ppt_temp, ppt_temp ) ;
 //printf("tess from xgeom, external vertex %g %g\n", x, y);

            }
            else
                  cntr[0]--;

            x0 = x;
            y0 = y;
      }

      gluTessEndContour(GLUtessobj);


      int index_offset = npte;
#if 1
//  Now the interior contours
      for(iir=0; iir < ncnt-1; iir++)
      {
            gluTessBeginContour(GLUtessobj);

//            int npti = cntr[iir];
            int npti  = m_pxgeom->contour_array[iir+1];

      //  Check and account for winding direction of ring
            bool cw = false; //!(poly->getInteriorRing(iir)->isClockwise() == 0);

            if(!cw)
            {
                  x0 = pp[index_offset].m_x;
                  y0 = pp[index_offset].m_y;
            }
            else
            {
                  x0 = pp[index_offset + npti-1].m_x;
                  y0 = pp[index_offset + npti-1].m_y;
            }

//  Transcribe points to vertex array, in proper order with no duplicates
//   also, accounting for tess_orient
            for(int ip = 0 ; ip < npti ; ip++)
            {
                  OGRPoint p;
                  int pidx;
                  if(!cw)                               // interior contours must be cw
                        pidx = npti - ip - 1;
                  else
                        pidx = ip;


                  x = pp[pidx + index_offset].m_x;
                  y = pp[pidx + index_offset].m_y;

                  if((fabs(x-x0) > EQUAL_EPS) || (fabs(y-y0) > EQUAL_EPS))
                  {
                        GLdouble *ppt_temp = ppt;
                        if(tess_orient == TESS_VERT)
                        {
                              *ppt++ = x;
                              *ppt++ = y;
                        }
                        else
                        {
                              *ppt++ = y;
                              *ppt++ = x;
                        }
                        *ppt++ = 0.0;

                        gluTessVertex( GLUtessobj, ppt_temp, ppt_temp ) ;
//      printf("tess from xgeom, internal vertex %d %g %g\n", ip, x, y);

                  }
                  else
                        cntr[iir+1]--;

                  x0 = x;
                  y0 = y;

            }

            gluTessEndContour(GLUtessobj);

            index_offset += npti;
      }
#endif

    //  Store some SM conversion data in static store,
    //  for callback access
      s_bSENC_SM =  false;

      s_bmerc_transform = true;
      s_transform_x_rate   =  m_pxgeom->x_rate;
      s_transform_x_origin =  m_pxgeom->x_offset;
      s_transform_y_rate   =  m_pxgeom->y_rate;
      s_transform_y_origin =  m_pxgeom->y_offset;


    //      Ready to kick off the tesselator

      s_pTPG_Last = NULL;
      s_pTPG_Head = NULL;

      s_nvmax = 0;

      gluTessEndPolygon(GLUtessobj);          // here it goes

      m_nvertex_max = s_nvmax;               // record largest vertex count, updates in callback


    //  Tesselation all done, so...

    //  Create the data structures

      m_ppg_head = new PolyTriGroup;
      m_ppg_head->m_bSMSENC = s_bSENC_SM;
      m_ppg_head->data_type = DATA_TYPE_DOUBLE;

      m_ppg_head->nContours = ncnt;
      m_ppg_head->pn_vertex = cntr;             // pointer to array of poly vertex counts


//  Transcribe the raw geometry buffer
//  Converting to float as we go, and
//  allowing for tess_orient
//  Also, convert to SM if requested

      int nptfinal = npta;

      //  No longer need the full geometry in the SENC,
      nptfinal = 1;

      m_nwkb = (nptfinal +1) * 2 * sizeof(float);
      m_ppg_head->pgroup_geom = (float *)malloc(m_nwkb);
      float *vro = m_ppg_head->pgroup_geom;
      ppt = geoPt;
      float tx,ty;

      for(ip = 0 ; ip < npta ; ip++)
      {
            if(TESS_HORZ == tess_orient)
            {
                  ty = *ppt++;
                  tx = *ppt++;
            }
            else
            {
                  tx = *ppt++;
                  ty = *ppt++;
            }

            if(0/*bSENC_SM*/)
            {
            //  Calculate SM from chart common reference point
                  double easting, northing;
//                  toSM(ty, tx, ref_lat, ref_lon, &easting, &northing);
                  *vro++ = easting;              // x
                  *vro++ = northing;             // y
            }
            else
            {
                  *vro++ = tx;                  // lon
                  *vro++ = ty;                  // lat
            }

            ppt++;                      // skip z
      }

      m_ppg_head->tri_prim_head = s_pTPG_Head;         // head of linked list of TriPrims

      //  Convert the Triangle vertex arrays into a single memory allocation of floats
      //  to reduce SENC size and enable efficient access later

      //  First calculate the total byte size
      int total_byte_size = 0;
      TriPrim *p_tp = m_ppg_head->tri_prim_head;
      while( p_tp ) {
          total_byte_size += p_tp->nVert * 2 * sizeof(float);
          p_tp = p_tp->p_next; // pick up the next in chain
      }

      float *vbuf = (float *)malloc(total_byte_size);
      p_tp = m_ppg_head->tri_prim_head;
      float *p_run = vbuf;
      while( p_tp ) {
          float *pfbuf = p_run;
          for( int i=0 ; i < p_tp->nVert * 2 ; ++i){
              float x = (float)(p_tp->p_vertex[i]);
              *p_run++ = x;
          }

          free(p_tp->p_vertex);
          p_tp->p_vertex = (double *)pfbuf;
          p_tp = p_tp->p_next; // pick up the next in chain
      }
      m_ppg_head->bsingle_alloc = true;
      m_ppg_head->single_buffer = (unsigned char *)vbuf;
      m_ppg_head->single_buffer_size = total_byte_size;
      m_ppg_head->data_type = DATA_TYPE_FLOAT;

      gluDeleteTess(GLUtessobj);

      free( s_pwork_buf );
      s_pwork_buf = NULL;

      free (geoPt);

      //    All allocated buffers are owned now by the m_ppg_head
      //    And will be freed on dtor of this object
      delete m_pxgeom;

      //      Free up any "Combine" vertices created
      for(unsigned int i = 0; i < s_pCombineVertexArray->GetCount() ; i++)
            free (s_pCombineVertexArray->Item(i));
      delete s_pCombineVertexArray;


      m_pxgeom = NULL;

      m_bOK = true;

#endif          //#ifdef ocpnUSE_GL

      return 0;
}





// GLU tesselation support functions
void __CALL_CONVENTION beginCallback(GLenum which)
{
    s_buf_idx = 0;
    s_nvcall = 0;
    s_gltri_type = which;
}

/*
void __CALL_CONVENTION errorCallback(GLenum errorCode)
{
    const GLubyte *estring;

    estring = gluErrorString(errorCode);
    printf("Tessellation Error: %s\n", estring);
    exit(0);
}
*/

void __CALL_CONVENTION endCallback(void)
{
    //      Create a TriPrim

    char buf[40];

    if(s_nvcall > s_nvmax)                            // keep track of largest number of triangle vertices
          s_nvmax = s_nvcall;

    switch(s_gltri_type)
    {
        case GL_TRIANGLE_FAN:
        case GL_TRIANGLE_STRIP:
        case GL_TRIANGLES:
        {
            TriPrim *pTPG = new TriPrim;
            if(NULL == s_pTPG_Last)
            {
                s_pTPG_Head = pTPG;
                s_pTPG_Last = pTPG;
            }
            else
            {
                s_pTPG_Last->p_next = pTPG;
                s_pTPG_Last = pTPG;
            }

            pTPG->p_next = NULL;
            pTPG->type = s_gltri_type;
            pTPG->nVert = s_nvcall;

        //  Calculate bounding box
            pTPG->p_bbox = new wxBoundingBox;

            float sxmax = -1000;                   // this poly BBox
            float sxmin = 1000;
            float symax = -90;
            float symin = 90;

            GLdouble *pvr = s_pwork_buf;
            for(int iv=0 ; iv < s_nvcall ; iv++)
            {
                GLdouble xd, yd;
                xd = *pvr++;
                yd = *pvr++;

                if(s_bmerc_transform)
                {
                      double valx = ( xd * s_transform_x_rate ) + s_transform_x_origin;
                      double valy = ( yd * s_transform_y_rate ) + s_transform_y_origin;

                      //    Convert to lat/lon
                      double lat = ( 2.0 * atan ( exp ( valy/CM93_semimajor_axis_meters ) ) - PI/2. ) / DEGREE;
                      double lon = ( valx / ( DEGREE * CM93_semimajor_axis_meters ) );

                      sxmax = fmax(lon, sxmax);
                      sxmin = fmin(lon, sxmin);
                      symax = fmax(lat, symax);
                      symin = fmin(lat, symin);
                }
                else
                {
                      sxmax = fmax(xd, sxmax);
                      sxmin = fmin(xd, sxmin);
                      symax = fmax(yd, symax);
                      symin = fmin(yd, symin);
                }
            }

            pTPG->p_bbox->SetMin(sxmin, symin);
            pTPG->p_bbox->SetMax(sxmax, symax);


            //  Transcribe this geometry to TriPrim, converting to SM if called for

            if(s_bSENC_SM)
            {
                double *pds = s_pwork_buf;
                pTPG->p_vertex = (double *)malloc(s_nvcall * 2 * sizeof(double));
                double *pdd = pTPG->p_vertex;

                for(int ip = 0 ; ip < s_nvcall ; ip++)
                {
                    double dlon = *pds++;
                    double dlat = *pds++;

                    double easting, northing;
                    toSM(dlat, dlon, s_ref_lat, s_ref_lon, &easting, &northing);
                    double deast = easting;
                    double dnorth = northing;
                    *pdd++ = deast;
                    *pdd++ = dnorth;
                }
            }
            else
            {
                pTPG->p_vertex = (double *)malloc(s_nvcall * 2 * sizeof(double));
                memcpy(pTPG->p_vertex, s_pwork_buf, s_nvcall * 2 * sizeof(double));
            }


            break;
        }
        default:
        {
            sprintf(buf, "....begin Callback  unknown\n");
            break;
        }
    }
}

void __CALL_CONVENTION vertexCallback(GLvoid *vertex)
{
    GLdouble *pointer;

    pointer = (GLdouble *) vertex;

    if(s_buf_idx > s_buf_len - 4)
    {
        int new_buf_len = s_buf_len + 100;
        GLdouble * tmp = s_pwork_buf;

        s_pwork_buf = (GLdouble *)realloc(s_pwork_buf, new_buf_len * sizeof(GLdouble));
        if (NULL == s_pwork_buf)
        {
            free(tmp);
            tmp = NULL;
        }
        else
            s_buf_len = new_buf_len;
    }

    if(tess_orient == TESS_VERT)
    {
        s_pwork_buf[s_buf_idx++] = pointer[0];
        s_pwork_buf[s_buf_idx++] = pointer[1];
    }
    else
    {
        s_pwork_buf[s_buf_idx++] = pointer[1];
        s_pwork_buf[s_buf_idx++] = pointer[0];
    }


    s_nvcall++;

}

/*  combineCallback is used to create a new vertex when edges
 *  intersect.
 */
void __CALL_CONVENTION combineCallback(GLdouble coords[3],
                     GLdouble *vertex_data[4],
                     GLfloat weight[4], GLdouble **dataOut )
{
    GLdouble *vertex = (GLdouble *)malloc(6 * sizeof(GLdouble));

    vertex[0] = coords[0];
    vertex[1] = coords[1];
    vertex[2] = coords[2];
    vertex[3] = vertex[4] = vertex[5] = 0. ; //01/13/05 bugfix

    *dataOut = vertex;

    s_pCombineVertexArray->Add(vertex);
}


#endif
wxStopWatch *s_stwt;

#if 0
//      Build Trapezoidal PolyTessGeoTrap Object from Extended_Geometry
PolyTessGeoTrap::PolyTessGeoTrap(Extended_Geometry *pxGeom)
{
      m_bOK = false;

      m_ptg_head = new PolyTrapGroup(pxGeom);
      m_nvertex_max = pxGeom->n_max_vertex;           // record the maximum number of segment vertices

      //    All allocated buffers are owned now by the m_ptg_head
      //    And will be freed on dtor of this object
      delete pxGeom;
}


PolyTessGeoTrap::~PolyTessGeoTrap()
{
      delete m_ptg_head;
}

void PolyTessGeoTrap::BuildTess()
{
           //    Flip the passed vertex array, contour-by-contour
      int offset = 1;
      for(int ict=0 ; ict < m_ptg_head->nContours ; ict++)
      {
            int nvertex = m_ptg_head->pn_vertex[ict];
/*
            for(int iv=0 ; iv < nvertex/2 ; iv++)
            {
                  wxPoint2DDouble a = m_ptg_head->ptrapgroup_geom[iv + offset];
                  wxPoint2DDouble b = m_ptg_head->ptrapgroup_geom[(nvertex - 1) - iv + offset];
                  m_ptg_head->ptrapgroup_geom[iv + offset] = b;
                  m_ptg_head->ptrapgroup_geom[(nvertex - 1) - iv + offset] = a;
            }
*/
            wxPoint2DDouble *pa = &m_ptg_head->ptrapgroup_geom[offset];
            wxPoint2DDouble *pb = &m_ptg_head->ptrapgroup_geom[(nvertex - 1) + offset];

            for(int iv=0 ; iv < nvertex/2 ; iv++)
            {

                  wxPoint2DDouble a = *pa;
                  *pa = *pb;
                  *pb = a;

                  pa++;
                  pb--;
            }

            offset += nvertex;
      }


      itrap_t *itr;
      isegment_t *iseg;
      int n_traps;

      int trap_err = int_trapezate_polygon(m_ptg_head->nContours, m_ptg_head->pn_vertex, (double (*)[2])m_ptg_head->ptrapgroup_geom, &itr, &iseg, &n_traps);

     m_ptg_head->m_trap_error = trap_err;

      if(0 != n_traps)
      {
       //  Now the Trapezoid Primitives

      //    Iterate thru the trapezoid structure counting valid, non-empty traps

            int nvtrap = 0;
            for(int it=1 ; it< n_traps ; it++)
            {
//            if((itr[i].state == ST_VALID) && (itr[i].hi.y != itr[i].lo.y) && (itr[i].lseg != -1) && (itr[i].rseg != -1) && (itr[i].inside == 1))
                  if(itr[it].inside == 1)
                        nvtrap++;
            }

            m_ptg_head->ntrap_count = nvtrap;

      //    Allocate enough memory
            m_ptg_head->trap_array = (trapz_t *)malloc(nvtrap * sizeof(trapz_t));

      //    Iterate again and capture the valid trapezoids
            trapz_t *prtrap = m_ptg_head->trap_array;
            for(int i=1 ; i< n_traps ; i++)
            {
//            if((itr[i].state == ST_VALID) && (itr[i].hi.y != itr[i].lo.y) && (itr[i].lseg != -1) && (itr[i].rseg != -1) && (itr[i].inside == 1))
                  if(itr[i].inside == 1)
                  {


                  //    Fix up the trapezoid segment indices to account for ring closure points in the input vertex array
                        int i_adjust = 0;
                        int ic = 0;
                        int pcount = m_ptg_head->pn_vertex[0]-1;
                        while(itr[i].lseg > pcount)
                        {
                              i_adjust++;
                              ic++;
                              if(ic >= m_ptg_head->nContours)
                                    break;
                              pcount += m_ptg_head->pn_vertex[ic]-1;
                        }
                        prtrap->ilseg = itr[i].lseg + i_adjust;


                        i_adjust = 0;
                        ic = 0;
                        pcount = m_ptg_head->pn_vertex[0]-1;
                        while(itr[i].rseg > pcount)
                        {
                              i_adjust++;
                              ic++;
                              if(ic >=  m_ptg_head->nContours)
                                    break;
                              pcount += m_ptg_head->pn_vertex[ic]-1;
                        }
                        prtrap->irseg = itr[i].rseg + i_adjust;

                  //    Set the trap y values

                        prtrap->hiy = itr[i].hi.y;
                        prtrap->loy = itr[i].lo.y;

                        prtrap++;
                  }
            }


      }     // n_traps_ok
      else
            m_nvertex_max = 0;



//  Free the trapezoid structure array
      free(itr);
      free(iseg);

      //    Always OK, even if trapezator code faulted....
      //    Contours should be OK, anyway, and    m_ptg_head->ntrap_count will be 0;

      m_bOK = true;

}


#endif

//------------------------------------------------------------------------------
//          PolyTriGroup Implementation
//------------------------------------------------------------------------------
PolyTriGroup::PolyTriGroup()
{
    pn_vertex = NULL;             // pointer to array of poly vertex counts
    pgroup_geom = NULL;           // pointer to Raw geometry, used for contour line drawing
    tri_prim_head = NULL;         // head of linked list of TriPrims
    m_bSMSENC = false;
    bsingle_alloc = false;
    single_buffer = NULL;
    single_buffer_size = 0;
    data_type = DATA_TYPE_DOUBLE;

}

PolyTriGroup::~PolyTriGroup()
{
    free(pn_vertex);
    free(pgroup_geom);
    //Walk the list of TriPrims, deleting as we go
    TriPrim *tp_next;
    TriPrim *tp = tri_prim_head;

    if(bsingle_alloc){
        free(single_buffer);
        while(tp) {
            tp_next = tp->p_next;
            delete tp;
            tp = tp_next;
        }
    }
    else {
        while(tp) {
            tp_next = tp->p_next;
            tp->FreeMem();
            delete tp;
            tp = tp_next;
        }
    }
}

//------------------------------------------------------------------------------
//          TriPrim Implementation
//------------------------------------------------------------------------------
TriPrim::TriPrim()
{
}

TriPrim::~TriPrim()
{
}

void TriPrim::FreeMem()
{
    free(p_vertex);
}


//------------------------------------------------------------------------------
//          PolyTrapGroup Implementation
//------------------------------------------------------------------------------
PolyTrapGroup::PolyTrapGroup()
{
      pn_vertex = NULL;             // pointer to array of poly vertex counts
      ptrapgroup_geom = NULL;           // pointer to Raw geometry, used for contour line drawing
      trap_array = NULL;            // pointer to trapz_t array

      ntrap_count = 0;
}

PolyTrapGroup::PolyTrapGroup(Extended_Geometry *pxGeom)
{
      m_trap_error = 0;

      nContours = pxGeom->n_contours;

      pn_vertex = pxGeom->contour_array;             // pointer to array of poly vertex counts
      pxGeom->contour_array = NULL;

      ptrapgroup_geom = pxGeom->vertex_array;
      pxGeom->vertex_array = NULL;

      ntrap_count = 0;                                // provisional
      trap_array = NULL;                              // pointer to generated trapz_t array
}

PolyTrapGroup::~PolyTrapGroup()
{
      free(pn_vertex);
      free(ptrapgroup_geom);
      free (trap_array);
}









