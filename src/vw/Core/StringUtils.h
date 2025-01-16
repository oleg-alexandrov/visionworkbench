/// \file Core/StringUtils.h
///
/// Minor string utilities. 
///
#ifndef __VW_CORE_STRINGUTILS_H__
#define __VW_CORE_STRINGUTILS_H__

#include <string>
#include <sstream>
#include <vector>
#include <vw/Core/Exception.h>

namespace vw{

  /// Convert a number to a string
  template <typename T>
  std::string num_to_str(T val, size_t precision=16);

  /// Pack a Vector into a string
  template <class VecT>
  std::string vec_to_str(VecT const& vec);

  /// Extract a string into a Vector of given size.
  template<class VecT>
  VecT str_to_vec(std::string const& str, std::string separators = "");

  /// Executes a find-replace operation in-place on a string.
  /// - Returns the number of instances replaced.
  size_t string_replace(std::string &s, std::string const& find, std::string const& replace);

  //=============================================================================
  // Function definitions

  template <typename T>
  std::string num_to_str(T val, size_t precision){

    std::ostringstream oss;
    oss.precision(precision);
    oss << val;
    return oss.str();
  }

  template <class VecT>
  std::string vec_to_str(VecT const& vec){

    // Use 17 digits of precision
    std::ostringstream oss;
    oss.precision(17);
    for (int i = 0; i < (int)vec.size()-1; i++) oss << vec[i] << " ";
    if (vec.size() > 0) oss << vec[vec.size()-1];
    
    return oss.str();
  }

  // Invoke this, for example, as:
  // Vector3 vals = str_to_vec(str);
  // or:
  // vals = str_to_vec<Vector3>(str);
  // Optionally pass in several separators (by default use space
  // as separator).
  // See below for parsing into an std vector.
  template<class VecT>
  VecT str_to_vec(std::string const& str, std::string separators) {

    std::istringstream iss;
    if (separators == "") {
      iss = std::istringstream(str);
    } else {
      // Replace the separators with space before tokenizing
      std::string proc_str = str;
      for (size_t it = 0; it < separators.size(); it++) {
        std::string find;
        find += separators[it];
        std::string replace = " ";
        string_replace(proc_str, find, replace);
      }
      iss = std::istringstream(proc_str);
    }
    
    VecT vec;
    for (int i = 0; i < (int)vec.size(); i++){
      if (! (iss >> vec[i]) )
        vw::vw_throw(vw::ArgumentErr() << "Failed to extract value from: " << str << "\n");
    }
    return vec;
  }

  // Parse a string with given separators into a vector of doubles.
  std::vector<double> str_to_std_vec(std::string const& str, std::string separators);

 // Parses a string containing a list of numbers separated by commas or spaces
 // TODO(oalexan1): Replace this with str_to_std_vec. Needs testing.
 void split_number_string(const std::string &input, std::vector<double> &output);

} // namespace vw

#endif // __VW_CORE_STRINGUTILS_H__
