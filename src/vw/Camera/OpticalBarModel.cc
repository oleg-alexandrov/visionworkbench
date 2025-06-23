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

#include <vw/Math/EulerAngles.h>
#include <vw/Camera/CameraModel.h>
#include <vw/Camera/CameraSolve.h>
#include <vw/Camera/OpticalBarModel.h>
#include <vw/Camera/OrbitalCorrections.h>
#include <vw/Math/NewtonRaphson.h>

#include <iomanip>

namespace vw {
namespace camera {

OpticalBarModel::OpticalBarModel(): 
  m_motion_compensation(1.0),
  m_correct_velocity_aberration(true),
  m_correct_atmospheric_refraction(true) {}

OpticalBarModel::OpticalBarModel(std::string const& path):
  m_motion_compensation(1.0),
  m_correct_velocity_aberration(true),
  m_correct_atmospheric_refraction(true) {
  // Create from file. This will read m_mean_earth_radius and m_mean_surface_elevation.    
  read(path);
} 

OpticalBarModel::OpticalBarModel(vw::Vector2i image_size,
                vw::Vector2  center_offset_pixels,
                double   pixel_size,
                double   focal_length,
                double   scan_time,
                bool     scan_left_to_right,
                double   forward_tilt_radians,
                vw::Vector3  initial_position,
                vw::Vector3  initial_orientation,
                double   speed,
                double   motion_compensation_factor):
    m_image_size          (image_size),
    m_center_loc_pixels   (center_offset_pixels),
    m_pixel_size          (pixel_size),
    m_focal_length        (focal_length),
    m_scan_time           (scan_time),
    m_scan_left_to_right  (scan_left_to_right),
    m_forward_tilt_radians(forward_tilt_radians),
    m_initial_position    (initial_position),
    m_initial_orientation (initial_orientation),
    m_speed               (speed),
    m_motion_compensation(motion_compensation_factor),
    m_correct_velocity_aberration(true),
    m_correct_atmospheric_refraction(true),
    m_mean_earth_radius(DEFAULT_EARTH_RADIUS),
    m_mean_surface_elevation(DEFAULT_SURFACE_ELEVATION) {
  
  compute_scan_rate();
}

Vector2 OpticalBarModel::pixel_to_sensor_plane(Vector2 const& pixel) const {
  Vector2 result = (pixel - m_center_loc_pixels) * m_pixel_size;
  return result;
}

double OpticalBarModel::sensor_to_alpha(vw::Vector2 const& sensor_loc) const {
  // This calculation comes from the focal point projected on to a circular surface.
  return sensor_loc[0] / m_focal_length;
}

void OpticalBarModel::compute_scan_rate() {

  // Compute the scan angle using pixel information.
  Vector2 p1 = pixel_to_sensor_plane(Vector2(0,0));
  Vector2 p2 = pixel_to_sensor_plane(Vector2(m_image_size - Vector2i(1,1)));
  
  double alpha1     = sensor_to_alpha(p1);
  double alpha2     = sensor_to_alpha(p2);
  double scan_angle = alpha2 - alpha1;
  
  m_scan_rate_radians = scan_angle / m_scan_time;
}

double OpticalBarModel::pixel_to_time_delta(Vector2 const& pix) const {

  // Since the camera sweeps a scan through columns, use that to
  //  determine the fraction of the way it is through the image.
  const int max_col = m_image_size[0]-1;
  double scan_fraction = 0;
  if (m_scan_left_to_right)
    scan_fraction = pix[0] / max_col; // TODO: Add 0.5 pixels to calculation?
  else // Right to left scan direction
    scan_fraction = (max_col - pix[0]) / max_col;
  double time_delta = scan_fraction * m_scan_time;
  return time_delta;
}

Vector3 OpticalBarModel::get_velocity(vw::Vector2 const& pixel) const {

  // TODO: For speed, store the pose*velocity vector.
  // Convert the velocity from sensor coords to GCC coords
  Matrix3x3 pose = camera_pose(pixel).rotation_matrix();

  // Recover the satellite attitude relative to the tilted camera position
  //Matrix3x3 m = vw::math::rotation_x_axis(-m_forward_tilt_radians)*pose;
  Matrix3x3 m = pose*vw::math::rotation_x_axis(m_forward_tilt_radians);
  
  return m*Vector3(0,m_speed,0);
}

Vector3 OpticalBarModel::camera_center(Vector2 const& pix) const {
  // We model with a constant velocity.
  double dt = pixel_to_time_delta(pix);

  return m_initial_position + dt*get_velocity(pix);
}


Quat OpticalBarModel::camera_pose(Vector2 const& pix) const {
  // Camera pose is treated as constant for the duration of a scan.
  return axis_angle_to_quaternion(m_initial_orientation);
}

Vector3 OpticalBarModel::pixel_to_vector_uncorrected(Vector2 const& pixel) const {
 
  Vector2 sensor_plane_pos = pixel_to_sensor_plane(pixel);
  Vector3 cam_center       = camera_center(pixel);
  Quat    cam_pose         = camera_pose  (pixel);

  // This is the horizontal angle away from the center point (from straight out of the camera)
  double alpha = sensor_to_alpha(sensor_plane_pos);

  // Distance from the camera center to the ground.
  double H = norm_2(cam_center) - (m_mean_surface_elevation + m_mean_earth_radius);

  // Distortion caused by compensation for the satellite's forward motion during the image.
  // - The film was actually translated underneath the lens to compensate for the motion.
  double image_motion_compensation = ((m_focal_length * m_speed) / (H*m_scan_rate_radians))
                                      * sin(alpha) * m_motion_compensation;
  if (!m_scan_left_to_right) // Sync alpha with motion compensation.
    image_motion_compensation *= -1.0;


  // This vector is ESD format, consistent with the linescan model.
  Vector3 r(m_focal_length * sin(alpha),
            sensor_plane_pos[1] + image_motion_compensation,
            m_focal_length * cos(alpha));
  r = normalize(r);

  // r is the ray vector in the local camera system

  // Convert the ray vector into GCC coordinates.
  Vector3 result = cam_pose.rotate(r);

  return result;
}

Vector3 OpticalBarModel::pixel_to_vector(Vector2 const& pixel) const {

  Vector3 output_vector;

  try {
    output_vector = pixel_to_vector_uncorrected(pixel);

    Vector3 cam_ctr = camera_center(pixel);
    if (m_correct_atmospheric_refraction) {
      vw::Quaternion<double> corr_rot; 
      output_vector
        = apply_atmospheric_refraction_correction(cam_ctr, m_mean_earth_radius,
                                                  m_mean_surface_elevation, output_vector, corr_rot);
    }
    
    if (m_correct_velocity_aberration) {
      vw::Quaternion<double> corr_rot;
      output_vector
        = apply_velocity_aberration_correction(cam_ctr, get_velocity(pixel),
                                               m_mean_earth_radius, output_vector, corr_rot);
    }
  } catch(const vw::Exception &e) {
    // Repackage any of our exceptions thrown below this point as a 
    //  pixel to ray exception that other code will be able to handle.
    vw_throw(vw::camera::PixelToRayErr() << e.what());
  }
  
  return output_vector;
}

// Find two vectors that are perpendicular to each other and to the input unit
// vector.
// TODO(oalexan1): Can move this to a lower-level location.
void findPerpVecs(vw::Vector3 const& vec,
                  vw::Vector3 & perp1, vw::Vector3 & perp2) {

   // The input vec must have norm of 1, with tolerance
   if (std::abs(norm_2(vec) - 1.0) > 1e-5)
     vw_throw(ArgumentErr() << "findPerpVecs: Input vector must be a unit vector.\n");
     
   // Find the smallest coordinate in vec
   int min_i = 0;
   for (int i = 1; i < 3; i++) {
     if (std::abs(vec[i]) < std::abs(vec[min_i]))
       min_i = i;
   }
   
   // Find the other two indices
   int j = 0, k = 0;
   if (min_i == 0) {
     j = 1; k = 2;
   } else if (min_i == 1) {
     j = 0; k = 2;
   } else { // min_i == 2
     j = 0; k = 1;
   }
   
   // Find the vector that swaps j and k and keeps min_i at 0. This is a vector
   // that is perpendicular to vec.
   perp1 = Vector3(0, 0, 0);
   perp1[min_i] = 0; // Keep the smallest coordinate at 0
   perp1[j] = -vec[k]; // Set the other two coordinates to be perpendicular
   perp1[k] =  vec[j]; // This is the swap, so it is perpendicular to vec

   // Normalize 
   perp1 = normalize(perp1);
    
  // The second perpendicular vector is the cross product of the two
  perp2 = cross_prod(vec, perp1);
  // Normalize 
  perp2 = normalize(perp2);
}

// Given a 3D point on the ground and a pixel in the camera, find the difference
// between the camera-to-ground vector and the pixel direction vector, then
// measure how much they differ in a plane at the ground level and approximately
// parallel to the ground. When that difference is zero, we found the camera
// pixel that corresponds to the ground point.
class LinescanErr {
  const CameraModel* m_model;
  Vector3 m_point;
  // Two vectors in plane that is mostly perpendicular to camera-to-ground vectors
  vw::Vector3 m_perp1, m_perp2;
public:

LinescanErr(const CameraModel* model, const vw::Vector3& pt, vw::Vector2 const& guess):
  m_model(model), m_point(pt) {
  
  // Find a direction from the camera to the ground
  vw::Vector3 cam_ctr = m_model->camera_center(guess);  
  vw::Vector3 ground_dir = normalize(m_point - cam_ctr);
  findPerpVecs(ground_dir, m_perp1, m_perp2);
}

// This must have the signature expected by Newton's method. Can throw
// exceptions. The math is described above.
vw::Vector2 operator()(vw::Vector2 const& pix) const {
  
  vw::Vector3 cam_ctr = m_model->camera_center(pix);
  
  // Normalized direction from camera to ground point
  double dist_to_ground = norm_2(m_point - cam_ctr);
  vw::Vector3 ground_dir = (m_point - cam_ctr) / dist_to_ground;
  
  // Normalized direction from pixel to ground
  vw::Vector3 pix_dir = m_model->pixel_to_vector(pix);
  
  vw::Vector3 diff = pix_dir - ground_dir;

  // Find the components on this in a plane that is mostly perpendicular to vectors
  // from the cameras to the ground, so the diff vector is projected onto this plane.
  double dot1 = dot_prod(diff, m_perp1);
  double dot2 = dot_prod(diff, m_perp2);
  
  // Multiply by dist to ground so we can measure these at ground level.
  dot1 *= dist_to_ground;
  dot2 *= dist_to_ground;

  return vw::Vector2(dot1, dot2);
}

}; // End class LinescanErr

// TODO(oalexan1): This could be sped up further, after putting in Newton's method as below.
// - The m_perp1 and m_perp2 vectors could be found once, when the object is created.
// - Then can implement the approach from the usgscsm linescan class, of finding an initial
//   affine transform for ground-to-image. 
// - Disable velocity aberration and atmospheric refraction corrections, as they are needed
//   only with high quality input geolocation, as here the camera is very rough.
Vector2 OpticalBarModel::point_to_pixel(Vector3 const& point) const {

  // Use the image center as the initial guess for the pixel
  Vector2 guess = m_image_size / 2.0;

  // The step size for numerical differentiation in pixel units. This is for
  // finding the Jacobian, so it need not be perfect. A small step size can
  // result in numerical error.   
  double step = 1e-2;

  // The error for this tol is measured at the ground level in LinescanErr.
  double tol = 1e-10;
  
  // Find pix so that err_func(pix) = ans, where guess is the pix initial guess
  LinescanErr err_func(this, point, guess);
  vw::math::NewtonRaphson nr(err_func);
  vw::Vector2 Y(0, 0); // Solve err_func(solution) = Y
  
  Vector2 solution2 = nr.solve(guess, Y, step, tol);
  
  return solution2;

}

void OpticalBarModel::apply_transform(vw::Matrix3x3 const & rotation,
                                      vw::Vector3   const & translation,
                                      double                scale) {
  // Extract current parameters
  vw::Vector3 position = this->camera_center();
  vw::Quat    pose     = this->camera_pose();

  vw::Quat rotation_quaternion(rotation);
  
  // New position and rotation
  position = scale*rotation*position + translation;
  pose     = rotation_quaternion*pose;

  this->set_camera_center(position);
  this->set_camera_pose  (pose.axis_angle());
}

void OpticalBarModel::read(std::string const& filename) {

  // Open the input file
  std::ifstream cam_file;
  cam_file.open(filename.c_str());
  if (cam_file.fail())
    vw_throw( IOErr() << "OpticalBarModel::read_file: Could not open file: " << filename );

  // Check for version number on the first line
  std::string line;
  std::getline(cam_file, line);
  if (line.find("VERSION") == std::string::npos) {
    vw_throw( IOErr() << "OpticalBarModel::read_file(): Version missing!\n" );
  }

  int file_version = 1;
  sscanf(line.c_str(),"VERSION_%d", &file_version); // Parse the version of the input file
  if (file_version < 4)
    vw_throw( ArgumentErr() << "OpticalBarModel::read_file(): Versions prior to 4 are not supported!\n" );

  // Read the camera type
  std::getline(cam_file, line);
  if (line.find("OPTICAL_BAR") == std::string::npos)
        vw_throw( ArgumentErr() << "OpticalBarModel::read_file: Expected OPTICAL_BAR type, but got type "
                                << line );

  // Start parsing all the parameters from the lines.
  std::getline(cam_file, line);
  if (!cam_file.good() || sscanf(line.c_str(),"image_size = %d %d",
      &m_image_size[0], &m_image_size[1]) != 2) {
    cam_file.close();
    vw_throw( IOErr() << "OpticalBarModel::read_file(): Could not read the image size\n" );
  }

  std::getline(cam_file, line);
  if (!cam_file.good() || sscanf(line.c_str(),"image_center = %lf %lf",
      &m_center_loc_pixels[0], &m_center_loc_pixels[1]) != 2) {
    cam_file.close();
    vw_throw( IOErr() << "OpticalBarModel::read_file(): Could not read the image center\n" );
  }

  std::getline(cam_file, line);
  if (!cam_file.good() || sscanf(line.c_str(),"pitch = %lf", &m_pixel_size) != 1) {
    cam_file.close();
    vw_throw( IOErr() << "OpticalBarModel::read_file(): Could not read the pixel pitch\n" );
  }

  std::getline(cam_file, line);
  if (!cam_file.good() || sscanf(line.c_str(),"f = %lf", &m_focal_length) != 1) {
    cam_file.close();
    vw_throw( IOErr() << "OpticalBarModel::read_file(): Could not read the focal_length\n" );
  }

  std::getline(cam_file, line);
  if (!cam_file.good() || sscanf(line.c_str(),"scan_time = %lf", &m_scan_time) != 1) {
    cam_file.close();
    vw_throw( IOErr() << "OpticalBarModel::read_file(): Could not read the scan time\n" );
  }

  std::getline(cam_file, line);
  if (!cam_file.good() || sscanf(line.c_str(),"forward_tilt = %lf", &m_forward_tilt_radians) != 1) {
    cam_file.close();
    vw_throw( IOErr() << "OpticalBarModel::read_file(): Could not read the forward tilt angle\n" );
  }
  
  std::getline(cam_file, line);
  if (!cam_file.good() || sscanf(line.c_str(),"iC = %lf %lf %lf", 
        &m_initial_position(0), &m_initial_position(1), &m_initial_position(2)) != 3) {
    cam_file.close();
    vw_throw( IOErr() << "OpticalBarModel::read_file(): Could not read the initial position\n" );
  }

  // Read and convert the rotation matrix.
  Matrix3x3 rot_mat;
  std::getline(cam_file, line);
  if ( !cam_file.good() ||
       sscanf(line.c_str(), "iR = %lf %lf %lf %lf %lf %lf %lf %lf %lf",
              &rot_mat(0,0), &rot_mat(0,1), &rot_mat(0,2),
              &rot_mat(1,0), &rot_mat(1,1), &rot_mat(1,2),
              &rot_mat(2,0), &rot_mat(2,1), &rot_mat(2,2)) != 9 ) {
    cam_file.close();
    vw_throw( IOErr() << "OpticalBarModel::read_file(): Could not read the rotation matrix\n" );
  }
  Quat q(rot_mat);
  m_initial_orientation = q.axis_angle();

  std::getline(cam_file, line);
  if (!cam_file.good() || sscanf(line.c_str(),"speed = %lf", &m_speed) != 1) {
    cam_file.close();
    vw_throw( IOErr() << "OpticalBarModel::read_file(): Could not read the speed\n" );
  }

  std::getline(cam_file, line);
  if (!cam_file.good() || sscanf(line.c_str(),"mean_earth_radius = %lf", &m_mean_earth_radius) != 1) {
    cam_file.close();
    vw_throw( IOErr() << "OpticalBarModel::read_file(): Could not read the mean earth radius\n" );
  }
  
  std::getline(cam_file, line);
  if (!cam_file.good() || sscanf(line.c_str(),"mean_surface_elevation = %lf", &m_mean_surface_elevation) != 1) {
    cam_file.close();
    vw_throw( IOErr() << "OpticalBarModel::read_file(): Could not read the mean surface elevation\n" );
  }

  std::getline(cam_file, line);
  if (!cam_file.good() || sscanf(line.c_str(),"motion_compensation_factor = %lf",
                                 &m_motion_compensation) != 1) {
    cam_file.close();
    vw_throw( IOErr() << "OpticalBarModel::read_file(): Could not read motion compensation factor\n" );
  }

  std::getline(cam_file, line);
  m_scan_left_to_right = line.find("scan_dir = left") == std::string::npos;

  cam_file.close();
  
  compute_scan_rate(); // This needs to be updated!
}

void OpticalBarModel::write(std::string const& filename) const {

  // TODO: Make compatible with .tsai files!

  // Open the output file for writing
  std::ofstream cam_file(filename.c_str());
  if( !cam_file.is_open() ) 
    vw_throw( IOErr() << "OpticalBarModel::write: Could not open file: " << filename );

  // Write the pinhole camera model parts
  //   # digits to survive double->text->double conversion
  const size_t ACCURATE_DIGITS = 17; // = std::numeric_limits<double>::max_digits10
  cam_file << std::setprecision(ACCURATE_DIGITS); 
  cam_file << "VERSION_4\n";
  cam_file << "OPTICAL_BAR\n";
  cam_file << "image_size = "   << m_image_size[0] << " " 
                                << m_image_size[1]<< "\n";
  cam_file << "image_center = " << m_center_loc_pixels[0] << " "
                                << m_center_loc_pixels[1] << "\n";
  cam_file << "pitch = "        << m_pixel_size             << "\n";
  cam_file << "f = "            << m_focal_length           << "\n";
  cam_file << "scan_time = "   << m_scan_time     << "\n";
  //cam_file << "scan_rate = "    << m_scan_rate_radians      << "\n";
  cam_file << "forward_tilt = " << m_forward_tilt_radians   << "\n";
  cam_file << "iC = " << m_initial_position[0] << " "
                      << m_initial_position[1] << " "
                      << m_initial_position[2] << "\n";
  // Store in the same format as the pinhole camera model.
  Matrix3x3 rot_mat = camera_pose(Vector2(0,0)).rotation_matrix();
  //cam_file << "iR = " << rot_mat[0] << " " rot_mat[1] << " " rot_mat[2] << " "
  //                    << m_initial_orientation[1] << " "
  //                    << m_initial_orientation[2] << "\n";
  cam_file << "iR = " << rot_mat(0,0) << " " << rot_mat(0,1) << " " << rot_mat(0,2) << " "
                      << rot_mat(1,0) << " " << rot_mat(1,1) << " " << rot_mat(1,2) << " "
                      << rot_mat(2,0) << " " << rot_mat(2,1) << " " << rot_mat(2,2) << "\n";
  cam_file << "speed = " << m_speed << "\n";
  cam_file << "mean_earth_radius = "          << m_mean_earth_radius      << "\n";
  cam_file << "mean_surface_elevation = "     << m_mean_surface_elevation << "\n";
  cam_file << "motion_compensation_factor = " << m_motion_compensation << "\n";
  if (m_scan_left_to_right)
    cam_file << "scan_dir = right\n";
  else
    cam_file << "scan_dir = left\n";
  cam_file.close();
}

std::ostream& operator<<( std::ostream& os, OpticalBarModel const& camera_model) {
  os << "\n------------------------ Optical Bar Model -----------------------\n\n";
  os << " Image size:             " << camera_model.m_image_size             << "\n";
  os << " Center loc (pixels):    " << camera_model.m_center_loc_pixels      << "\n";
  os << " Pixel size (m):         " << camera_model.m_pixel_size             << "\n";
  os << " Focal length (m):       " << camera_model.m_focal_length           << "\n";
  os << " Scan time (s):          " << camera_model.m_scan_time              << "\n";
  os << " Scan rate (rad/s):      " << camera_model.m_scan_rate_radians      << "\n";
  os << " Forward tilt (rad):     " << camera_model.m_forward_tilt_radians   << "\n";
  os << " Initial position:       " << camera_model.m_initial_position       << "\n";
  os << " Initial pose:           " << camera_model.m_initial_orientation    << "\n";
  os << " Speed:                  " << camera_model.m_speed                  << "\n";
  os << " Mean earth radius:      " << camera_model.m_mean_earth_radius      << "\n";
  os << " Mean surface elevation: " << camera_model.m_mean_surface_elevation << "\n";
  os << " Motion comp factor:     " << camera_model.m_motion_compensation<< "\n";
  os << " Left to right scan:     " << camera_model.m_scan_left_to_right     << "\n";
  os << "\n------------------------------------------------------------------------\n\n";
  return os;
}

}} // namespace asp::camera

