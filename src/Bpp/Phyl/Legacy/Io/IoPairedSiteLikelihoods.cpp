// SPDX-FileCopyrightText: The Bio++ Development Group
//
// SPDX-License-Identifier: CECILL-2.1

// From the STL
#include <vector>
#include <numeric>

// From Bio++
#include <Bpp/Text/TextTools.h>
#include "../Likelihood/PairedSiteLikelihoods.h"

#include "IoPairedSiteLikelihoods.h"

using namespace std;
using namespace bpp;

/*
 * Read from a stream in Tree-puzzle, phylip-like format
 */
PairedSiteLikelihoods IOTreepuzzlePairedSiteLikelihoods::readPairedSiteLikelihoods(std::istream& is)
{
  // The first line contains the number of models and the number of sites
  string line;
  getline(is, line);
  istringstream iss (line);
  size_t nmodels, nsites;
  iss >> nmodels;
  iss >> nsites;

  // Then each line contains a model name and site loglikelihoods under this model
  vector<string> names;
  vector<vector<double>> loglikelihoods;
  loglikelihoods.reserve(nmodels);

  // The exact format is determined upon the first line
  // Name and likelihoods fields can be delimited by either a tab or two spaces
  string delim;
  streampos pos (is.tellg());
  getline(is, line);
  if (line.find("\t") != string::npos)
    delim = "\t";
  else if (line.find("  ") != string::npos)
    delim = "  ";
  else
    throw;
  is.seekg(pos);

  // Read the data
  while (getline(is, line))
  {
    size_t delim_pos (line.find(delim));
    if (delim_pos == string::npos)
    {
      ostringstream msg;
      msg << "IOTreepuzzlePairedSiteLikelihoods::read: Couldn't find delimiter. The beggining of the line was : "
          << endl << line.substr(0, 100);
      throw Exception(msg.str());
    }

    // The name
    names.push_back( TextTools::removeSurroundingWhiteSpaces(line.substr(0, delim_pos)) );

    // The sites
    loglikelihoods.push_back(vector<double>());
    loglikelihoods.back().reserve(nsites);

    istringstream liks_stream ( line.substr(delim_pos) );
    double currllik;
    while (liks_stream >> currllik)
      loglikelihoods.back().push_back(currllik);

    // Check the number of sites
    if (loglikelihoods.back().size() != nsites)
    {
      ostringstream oss;
      oss << "IOTreepuzzlePairedSiteLikelihoods::read: Model '" << names.back()
          << "' does not have the correct number of sites. ("
          << loglikelihoods.back().size() << ", expected: " << nsites << ")";
      throw Exception(oss.str());
    }
  }

  // Check the number of models
  if (loglikelihoods.size() != nmodels)
    throw Exception("IOTreepuzzlePairedSiteLikelihoods::read: Wrong number of models.");

  PairedSiteLikelihoods psl (loglikelihoods, names);

  return psl;
}

/*
 * Read from a file in Tree-puzzle, phylip-like format
 */
PairedSiteLikelihoods IOTreepuzzlePairedSiteLikelihoods::readPairedSiteLikelihoods(const std::string& path)
{
  ifstream iF (path.c_str());
  PairedSiteLikelihoods psl (IOTreepuzzlePairedSiteLikelihoods::readPairedSiteLikelihoods(iF));
  return psl;
}

/*
 * Write to stream in Tree-puzzle, phylip-like format
 */
void IOTreepuzzlePairedSiteLikelihoods::writePairedSiteLikelihoods(const bpp::PairedSiteLikelihoods& psl, ostream& os, const string& delim)
{
  if (psl.getLikelihoods().size() == 0)
    throw Exception("Writing an empty PairedSiteLikelihoods object to file.");

  // Header line
  os << psl.getNumberOfModels() << " " << psl.getNumberOfSites() << endl;

  // Data lines

  if (delim == "\t")
  {
    // The delimiter is a tab
    for (size_t i = 0; i < psl.getNumberOfModels(); ++i)
    {
      os << psl.getModelNames().at(i) << "\t";
      for (vector<double>::const_iterator sitelik = psl.getLikelihoods().at(i).begin();
          sitelik != psl.getLikelihoods().at(i).end();
          ++sitelik)
      {
        if (sitelik == psl.getLikelihoods().at(i).end() - 1)
          os << *sitelik;
        else
          os << *sitelik << " ";
      }
      os << endl;
    }
  }

  else if (delim == "  ")
  // The delimiter is two spaces
  {
    // First we must get the length of the names field
    vector<size_t> name_sizes;
    for (vector<string>::const_iterator name = psl.getModelNames().begin();
        name != psl.getModelNames().end();
        ++name)
    {
      name_sizes.push_back(name->size());
    }
    size_t names_field_size = *max_element(name_sizes.begin(), name_sizes.end()) + 2;

    // Data lines
    for (size_t i = 0; i < psl.getNumberOfModels(); ++i)
    {
      // name field
      string name (psl.getModelNames().at(i));
      while (name.size() != names_field_size)
        name.append(" ");
      os << name;

      // site-likelihoods field
      for (vector<double>::const_iterator sitelik = psl.getLikelihoods().at(i).begin();
          sitelik != psl.getLikelihoods().at(i).end();
          ++sitelik)
      {
        if (sitelik == psl.getLikelihoods().at(i).end() - 1)
          os << *sitelik;
        else
          os << *sitelik << " ";
      }
      os << endl;
    }
  }

  else
  {
    ostringstream msg;
    msg << "IOTreepuzzlePairedSiteLikelihoods::write: Unknown field delimiter "
        << "\"" << delim << "\".";
    os << msg.str() << endl;
    throw Exception(msg.str());
  }
}


/*
 * Write to file in Tree-puzzle, phylip-like format
 */
void IOTreepuzzlePairedSiteLikelihoods::writePairedSiteLikelihoods(const bpp::PairedSiteLikelihoods& psl, const std::string& path, const string& delim)
{
  ofstream oF (path.c_str());
  IOTreepuzzlePairedSiteLikelihoods::writePairedSiteLikelihoods(psl, oF, delim);
}


/*
 * Read from a stream in Phyml format
 */
vector<double> IOPhymlPairedSiteLikelihoods::readPairedSiteLikelihoods(std::istream& is)
{
  vector<double> loglikelihoods;
  string str;

  // check the format, with first line
  getline(is, str);
  string expected ("Note : P(D|M) is the probability of site D given the model M (i.e., the site likelihood)");
  if (str.find(expected) == string::npos) // \r\n handling
  {
    ostringstream msg;
    msg << "IOPhymlPairedSiteLikelihoods::read: The first line was expected to be :"
        << expected << endl
        << "and was :" << endl
        << str << endl;
    throw Exception(msg.str());
  }

  // skip header lines
  for (int i = 0; i < 6; ++i)
  {
    getline(is, str);
  }

  while (getline(is, str))
  {
    // the site likelihood is the second field on the line
    istringstream ss (str);
    ss >> str;
    double lik;
    ss >> lik;
    loglikelihoods.push_back(log(lik));
  }

  return loglikelihoods;
}

/*
 * Read from a file in Phyml format
 */
vector<double> IOPhymlPairedSiteLikelihoods::readPairedSiteLikelihoods(const std::string& path)
{
  ifstream iF (path.c_str());
  vector<double> loglikelihoods(IOPhymlPairedSiteLikelihoods::readPairedSiteLikelihoods(iF));
  return loglikelihoods;
}
