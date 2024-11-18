// __BEGIN_LICENSE__
//  Copyright (c) 2006-2013, United States Government as represented by the
//  Administrator of the National Aeronautics and Space Administration. All
//  rights reserved.
//
//  The NASA Vision Workbench is licensed under the Apache License,
//  Version 2.0 (the "License"); you may not use this file except in
//  compliance with the License. You may obtain a copy of the License at
//  http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
// __END_LICENSE__

#include <vw/FileIO/FileUtils.h>
#include <vw/Cartography/shapeFile.h>
#include <vw/Core/Log.h>

#include <vector>
#include <algorithm>
#include <iostream>
#include <cassert>
#include <cfloat>
#include <cassert>
#include <cstring>
#include <string>
#include <map>

#include <boost/filesystem/path.hpp>

namespace vw { namespace geometry {

  // Convert a single point to OGRPoint
  void toOGR(double x, double y, OGRPoint & P){
    P = OGRPoint(x, y);
  }

  // Convert a polygonal line to OGR
  void toOGR(const double * xv, const double * yv, int startPos, int numVerts,
	     OGRLineString & L){

    L = OGRLineString(); // init

    for (int vIter = 0; vIter < numVerts; vIter++){
      double x = xv[startPos + vIter], y = yv[startPos + vIter];
      L.addPoint(x, y);
    }
  
    // A line string must have at least 2 points
    if (L.getNumPoints() <= 1) 
      L = OGRLineString(); 
  }

  // Convert a single polygon in a set of polygons to an ORG ring  
  void toOGR(const double * xv, const double * yv, int startPos, int numVerts,
	     OGRLinearRing & R){

    R = OGRLinearRing(); // init

    for (int vIter = 0; vIter < numVerts; vIter++){
      double x = xv[startPos + vIter], y = yv[startPos + vIter];
      R.addPoint(x, y);
    }
  
    // An OGRLinearRing must end with the same point as what it starts with
    double x = xv[startPos], y = yv[startPos];
    if (numVerts >= 2 &&
        x == xv[startPos + numVerts - 1] &&
        y == yv[startPos + numVerts - 1]) {
      // Do nothing, the polygon already starts and ends with the same point 
    }else{
      // Ensure the ring is closed
      R.addPoint(x, y);
    }

    // A ring must have at least 4 points (but the first is same as last)
    if (R.getNumPoints() <= 3) 
      R = OGRLinearRing(); 
  
  }

  // Convert a set of polygons to OGR
  void toOGR(vw::geometry::dPoly const& poly, OGRPolygon & P){

    P = OGRPolygon(); // reset

    const double * xv        = poly.get_xv();
    const double * yv        = poly.get_yv();
    const int    * numVerts  = poly.get_numVerts();
    int numPolys             = poly.get_numPolys();
  
    // Iterate over polygon rings, adding them one by one
    int startPos = 0;
    for (int pIter = 0; pIter < numPolys; pIter++){
    
      if (pIter > 0) startPos += numVerts[pIter - 1];
      int numCurrPolyVerts = numVerts[pIter];
    
      OGRLinearRing R;
      toOGR(xv, yv, startPos, numCurrPolyVerts, R);

      if (R.getNumPoints() >= 4){
        if (P.addRing(&R) != OGRERR_NONE)
          vw_throw(ArgumentErr() << "Failed add ring to polygon.\n");
      }
    }
  
    return;
  }

  // Extract a polygon from OGR
  void fromOGR(OGRPolygon *poPolygon, std::string const& poly_color,
               std::string const& layer_str, vw::geometry::dPoly & poly){

    bool isPolyClosed = true; // only closed polygons are supported
  
    poly.reset();
  
    int numInteriorRings = poPolygon->getNumInteriorRings();
  
    // Read exterior and interior rings
    int count = -1;
    while (1){
    
      count++;
      OGRLinearRing *ring;
    
      if (count == 0){
        // Exterior ring
        ring = poPolygon->getExteriorRing();
        if (ring == NULL || ring->IsEmpty ()){
          // No exterior ring, that means no polygon
          break;
        }
      }else{
        // Interior rings
        int iRing = count - 1;
        if (iRing >= numInteriorRings)
          break; // no more rings
        ring = poPolygon->getInteriorRing(iRing);
        if (ring == NULL || ring->IsEmpty ()) continue; // go to the next ring
      }
    
      int numPoints = ring->getNumPoints();
      std::vector<double> x, y;
      x.clear(); y.clear();
      for (int iPt = 0; iPt < numPoints; iPt++){
        OGRPoint poPoint;
        ring->getPoint(iPt, &poPoint);
        x.push_back(poPoint.getX());
        y.push_back(poPoint.getY());
      }

      // Don't record the last element if the same as the first
      int len = x.size();
      if (len >= 2 && x[0] == x[len-1] && y[0] == y[len-1]){
        len--;
        x.resize(len);
        y.resize(len);
      }
    
      poly.appendPolygon(len, vw::geometry::vecPtr(x), vw::geometry::vecPtr(y),
                         isPolyClosed, poly_color, layer_str);
    
    }
  }
  
  // Extract a polygonal line (line string) from OGR
  void fromOGR(OGRLineString *poLineString, std::string const& poly_color,
               std::string const& layer_str, vw::geometry::dPoly & poly){

    bool isPolyClosed = false; // polygonal lines are not closed
  
    poly.reset();

    std::vector<double> x, y;
    x.clear(); y.clear();
    
    int NumberOfVertices = poLineString ->getNumPoints();
    for (int k = 0; k < NumberOfVertices; k++) {

      OGRPoint P;
      poLineString ->getPoint(k, &P);
      x.push_back(P.getX());
      y.push_back(P.getY());
    }

    poly.appendPolygon(x.size(), vw::geometry::vecPtr(x), vw::geometry::vecPtr(y),
                       isPolyClosed, poly_color, layer_str);
    
  }

  // Extract a multi-polygon from OGR
  void fromOGR(OGRMultiPolygon *poMultiPolygon, std::string const& poly_color,
               std::string const& layer_str, std::vector<vw::geometry::dPoly> & polyVec,
               bool append){

    if (!append) polyVec.clear();

    int numGeom = poMultiPolygon->getNumGeometries();
    for (int iGeom = 0; iGeom < numGeom; iGeom++){
    
      const OGRGeometry *currPolyGeom = poMultiPolygon->getGeometryRef(iGeom);
      if (wkbFlatten(currPolyGeom->getGeometryType()) != wkbPolygon) continue;
    
      OGRPolygon *poPolygon = (OGRPolygon*) currPolyGeom;
      vw::geometry::dPoly poly;
      fromOGR(poPolygon, poly_color, layer_str, poly);
      polyVec.push_back(poly);
    }
  }

  // Read polygons from OGR geometry
  void fromOGR(OGRGeometry *poGeometry, std::string const& poly_color,
               std::string const& layer_str, std::vector<vw::geometry::dPoly> & polyVec,
               bool append){
  
    if (!append) polyVec.clear();
  
    if (poGeometry == NULL) {
    
      // nothing to do
    
    } else if (wkbFlatten(poGeometry->getGeometryType()) == wkbPoint) {
    
      // Create a polygon with just one point
      OGRPoint *poPoint = (OGRPoint*)poGeometry;
      std::vector<double> x, y;
      x.push_back(poPoint->getX());
      y.push_back(poPoint->getY());
      
      vw::geometry::dPoly poly;
      bool isPolyClosed = true; // only closed polygons are supported
      poly.setPolygon(x.size(), vw::geometry::vecPtr(x), vw::geometry::vecPtr(y),
                      isPolyClosed, poly_color, layer_str);
      polyVec.push_back(poly);
    
    } else if (wkbFlatten(poGeometry->getGeometryType()) == wkbMultiPolygon){
      bool append = true; 
      OGRMultiPolygon *poMultiPolygon = (OGRMultiPolygon*)poGeometry;
      fromOGR(poMultiPolygon, poly_color, layer_str, polyVec, append);
    
    } else if (wkbFlatten(poGeometry->getGeometryType()) == wkbPolygon){
      OGRPolygon *poPolygon = (OGRPolygon*)poGeometry;
      vw::geometry::dPoly poly;
      fromOGR(poPolygon, poly_color, layer_str, poly);
      polyVec.push_back(poly);
    
    } else if (wkbFlatten (poGeometry ->getGeometryType()) == wkbLineString) {
      OGRLineString *poLineString = (OGRLineString*)poGeometry;
      vw::geometry::dPoly poly;
      fromOGR(poLineString, poly_color, layer_str, poly);
      polyVec.push_back(poly);
    }
  }

  // Merge polygons into polyVec. The inputs are in ogr_polys, but they be
  // changed by this function. The output is in polyVec. 
  void mergeOGRPolygons(std::string const& poly_color, 
                        std::string const& layer_str,
                        std::vector<OGRGeometry*>& ogr_polys, 
                        std::vector<vw::geometry::dPoly>& polyVec) {
      
    // The doc of this function says that the elements in ogr_polys will
    // be taken care of. We are responsible only for the vector of pointers
    // and for the output of this function.
    //beg
    int pbIsValidGeometry = 0;
    const char** papszOptions = NULL;
    OGRGeometry* good_geom
      = OGRGeometryFactory::organizePolygons(vw::geometry::vecPtr(ogr_polys),
                                              ogr_polys.size(),
                                              &pbIsValidGeometry,
                                              papszOptions);

    // Single polygon, nothing to do
    if (wkbFlatten(good_geom->getGeometryType()) == wkbPolygon ||
        wkbFlatten(good_geom->getGeometryType()) == wkbPoint) {
      bool append = false;
      fromOGR(good_geom, poly_color, layer_str, polyVec, append);
    } else if (wkbFlatten(good_geom->getGeometryType()) == wkbMultiPolygon) {

      // We can merge
      OGRGeometry * merged_geom = new OGRPolygon;

      OGRMultiPolygon *poMultiPolygon = (OGRMultiPolygon*)good_geom;

      int numGeom = poMultiPolygon->getNumGeometries();
      for (int iGeom = 0; iGeom < numGeom; iGeom++) {

        const OGRGeometry *currPolyGeom = poMultiPolygon->getGeometryRef(iGeom);
        if (wkbFlatten(currPolyGeom->getGeometryType()) != wkbPolygon) continue;

        OGRPolygon *poPolygon = (OGRPolygon *) currPolyGeom;
        OGRGeometry * local_merged = merged_geom->Union(poPolygon);

        // Keep the pointer to the new geometry
        if (merged_geom != NULL)
          OGRGeometryFactory::destroyGeometry(merged_geom);
        merged_geom = local_merged;
      }

      bool append = false;
      fromOGR(merged_geom, poly_color, layer_str, polyVec, append);
      OGRGeometryFactory::destroyGeometry(merged_geom);
    }

    OGRGeometryFactory::destroyGeometry(good_geom);
  }

  // Read a shapefile
  void read_shapefile(std::string const& file,
                      std::string const& poly_color,
                      bool & has_geo, 
                      vw::cartography::GeoReference & geo,
                      std::vector<vw::geometry::dPoly> & polyVec){
  
    // Make sure the outputs are initialized
    has_geo = false;
    geo = vw::cartography::GeoReference();
    polyVec.clear();
  
    std::string layer_str = boost::filesystem::path(file).stem().string();

    vw_out() << "Reading layer: " << layer_str << "\n";
  
    GDALAllRegister();
    GDALDataset * poDS;
    poDS = (GDALDataset*) GDALOpenEx(file.c_str(), GDAL_OF_VECTOR, NULL, NULL, NULL);
    if (poDS == NULL) 
      vw_throw(ArgumentErr() << "Could not open file: " << file << ".\n");
  
    OGRLayer  *poLayer;
    poLayer = poDS->GetLayerByName(layer_str.c_str());
    if (poLayer == NULL)
      vw_throw(ArgumentErr() << "Could not find layer " << layer_str << " in file: "
               << file << ".\n");

    // Read the georef
    int nGeomFieldCount = poLayer->GetLayerDefn()->GetGeomFieldCount();
    char *pszWKT = NULL;
    if (nGeomFieldCount > 1) {
      for(int iGeom = 0; iGeom < nGeomFieldCount; iGeom ++){
        OGRGeomFieldDefn* poGFldDefn =
          poLayer->GetLayerDefn()->GetGeomFieldDefn(iGeom);
        OGRSpatialReference* poSRS = (OGRSpatialReference*)poGFldDefn->GetSpatialRef();
        if (poSRS == NULL)
          pszWKT = CPLStrdup("(unknown)");
        else {
          has_geo = true;
          poSRS->exportToPrettyWkt(&pszWKT);
          // Stop at the first geom
          break;
        }
      }
    }else{
      if (poLayer->GetSpatialRef() == NULL)
        pszWKT = CPLStrdup("(unknown)");
      else{
        has_geo = true;
        poLayer->GetSpatialRef()->exportToPrettyWkt(&pszWKT);
      }
    }
    geo.set_wkt(pszWKT);
    if (pszWKT != NULL)
      CPLFree(pszWKT);

    // There is no georef per se, as there is no image. The below forces
    // that the map from projected coordinates to pixel coordinates (point_to_pixel())
    // to be the identity.
    geo.set_pixel_interpretation(vw::cartography::GeoReference::PixelAsPoint);

    OGRFeature *poFeature;
    poLayer->ResetReading();
    while ((poFeature = poLayer->GetNextFeature()) != NULL) {
      OGRGeometry *poGeometry = poFeature->GetGeometryRef();
      bool append = true; 
      fromOGR(poGeometry, poly_color, layer_str, polyVec, append);
      OGRFeature::DestroyFeature(poFeature);
    }

    GDALClose(poDS);
  }
  
  // Write a shapefile
  void write_shapefile(std::string const& file,
                       bool has_geo,
                       vw::cartography::GeoReference const& geo, 
                       std::vector<vw::geometry::dPoly> const& polyVec){

    vw::create_out_dir(file);
    
    std::string layer_str = boost::filesystem::path(file).stem().string();

    const char *pszDriverName = "ESRI Shapefile";
    GDALDriver *poDriver;
    GDALAllRegister();
    poDriver = GetGDALDriverManager()->GetDriverByName(pszDriverName);
    if (poDriver == NULL) 
      vw_throw(ArgumentErr() << "Could not find driver: " << pszDriverName << ".\n");

    GDALDataset *poDS;
    poDS = poDriver->Create(file.c_str(), 0, 0, 0, GDT_Unknown, NULL);
    if (poDS == NULL) 
      vw_throw(ArgumentErr() << "Failed writing file: " << file << ".\n");
  
    // Write the georef
    OGRSpatialReference spatial_ref;
    OGRSpatialReference * spatial_ref_ptr = NULL;
    if (has_geo){
      std::string srs_string = geo.get_wkt();
      if (spatial_ref.SetFromUserInput(srs_string.c_str()))
        vw_throw(ArgumentErr() << "Failed to parse: \"" << srs_string << "\".");
      spatial_ref_ptr = &spatial_ref;
    }

    // Peek at the polygons and see the min and max number of vertices for each.
    int min_verts = 0, max_verts = 0;
    for (size_t vecIter = 0; vecIter < polyVec.size(); vecIter++){
      
      vw::geometry::dPoly const& poly = polyVec[vecIter]; // alias

      const double * xv        = poly.get_xv();
      const double * yv        = poly.get_yv();
      const int    * numVerts  = poly.get_numVerts();
      int numPolys             = poly.get_numPolys();
      
      int startPos = 0;
      for (int pIter = 0; pIter < numPolys; pIter++){
        if (pIter > 0) startPos += numVerts[pIter - 1];
        int num_curr_verts = numVerts[pIter];

        if (min_verts == 0 && max_verts == 0) {
          min_verts = num_curr_verts;
          max_verts = num_curr_verts;
        } else{
          min_verts = std::min(min_verts, num_curr_verts); 
          max_verts = std::max(max_verts, num_curr_verts);
        }
      }
    }
    
    // Create either a layer of polygons, lines, or points. A
    // shapefile cannot mix these.
    OGRLayer *polyLayer = NULL;
    if (min_verts == 1 && max_verts == 1) 
      polyLayer = poDS->CreateLayer(layer_str.c_str(), spatial_ref_ptr, wkbPoint, NULL);
    else if (min_verts == 2 && max_verts == 2)
      polyLayer = poDS->CreateLayer(layer_str.c_str(), spatial_ref_ptr, wkbLineString, NULL);
    else {
      if (min_verts <= 2)
        vw_out() << "Polygons with less than 3 vertices will not be saved.\n";
      polyLayer = poDS->CreateLayer(layer_str.c_str(), spatial_ref_ptr, wkbPolygon, NULL);
    }
      
    if (polyLayer == NULL)
      vw_throw(ArgumentErr() << "Failed creating layer: " << layer_str << ".\n");

    // Iterate over the vector of polygons
    for (size_t vecIter = 0; vecIter < polyVec.size(); vecIter++){

      vw::geometry::dPoly const& poly = polyVec[vecIter]; // alias
      if (poly.get_totalNumVerts() == 0) continue;

      const double * xv        = poly.get_xv();
      const double * yv        = poly.get_yv();
      const int    * numVerts  = poly.get_numVerts();
      int numPolys             = poly.get_numPolys();
      
      // Iterate over polygon rings, adding them one by one
      int startPos = 0;
      for (int pIter = 0; pIter < numPolys; pIter++){
        
        if (pIter > 0) startPos += numVerts[pIter - 1];
        int numCurrPolyVerts = numVerts[pIter];

        OGRFeature *poFeature = OGRFeature::CreateFeature(polyLayer->GetLayerDefn());
        
        if (numCurrPolyVerts >= 3) {  // Save a polygon
          
          OGRLinearRing R; // Form a ring
          toOGR(xv, yv, startPos, numCurrPolyVerts, R);
          OGRPolygon P; // Form a polygon with that ring
          if (R.getNumPoints() >= 4){
            if (P.addRing(&R) != OGRERR_NONE)
              vw_throw(ArgumentErr() << "Failed add ring to polygon.\n");
          }
          poFeature->SetGeometry(&P); // Form the feature

        } else if (min_verts == 2 && max_verts == 2) {

          OGRLineString L;
          toOGR(xv, yv, startPos, numCurrPolyVerts, L);
          poFeature->SetGeometry(&L); // Form the feature
          
        } else if (min_verts == 1 && max_verts == 1) { // Save a point
          
          OGRPoint P;
          toOGR(xv[startPos], yv[startPos], P);
          poFeature->SetGeometry(&P); // Form the feature
          
        }
        
        // Add the feature to the layer
        if (polyLayer->CreateFeature(poFeature) != OGRERR_NONE)
          vw_throw(ArgumentErr() << "Failed to create feature in shape file.\n");
        
        // Wipe the feature
        OGRFeature::DestroyFeature(poFeature);
    
      }
    }
    
    GDALClose(poDS);
  }

  // Bounding box of a shapefile
  void shapefile_bdbox(const std::vector<vw::geometry::dPoly> & polyVec,
                       // outputs
                       double & xll, double & yll,
                       double & xur, double & yur){
  
    double big = std::numeric_limits<double>::max();
    xll = big; yll = big; xur = -big; yur = -big;
    for (size_t p = 0; p < polyVec.size(); p++){
      if (polyVec[p].get_totalNumVerts() == 0) continue;
      double xll0, yll0, xur0, yur0;
      polyVec[p].bdBox(xll0, yll0, xur0, yur0);
      xll = std::min(xll, xll0); xur = std::max(xur, xur0);
      yll = std::min(yll, yll0); yur = std::max(yur, yur0);
    }

    return;
  }

}}
