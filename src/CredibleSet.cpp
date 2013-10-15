#include "CredibleSet.hpp"

#include <algorithm>
#include <map>

CredibleSet::CredibleSet(std::string file)
  : bipEx(std::vector<std::string>{file})
{
  bipEx.extractBipsNew(true);
  const auto &hash = bipEx.getBipartitionHashes()[0]; 
  totalTrees = hash.getTreesAdded(); 
} 


void CredibleSet::printCredibleSet(std::string filename, double thresh)
{
  const auto& hash = bipEx.getBipartitionHashes()[0]; 
  auto numBip = bipEx.getTaxa().size() - 3; 

  // sort the elems in the hash by number of occurrences of the bipartition 
  auto sortedBipOcc = std::vector<std::pair<Bipartition,Bipartition>>{}; 
  for(auto elem : hash)    
    {
      if(elem.first.count() > 1 )
	sortedBipOcc.push_back(std::make_pair(elem.first, elem.second)); 
    }

  auto sortFun = [](const std::pair<Bipartition, Bipartition> &elemA, 
  		    const std::pair<Bipartition, Bipartition> &elemB)
    {
      return elemA.second.count() > elemB.second.count() ; 
    }; 
  std::sort(sortedBipOcc.begin(), sortedBipOcc.end(), sortFun ); 

  auto hashFun = [](const std::vector<nat> &tree)
    {
      nat result = 0; 
      for(nat i = 0; i < tree.size() ;++i)
	result ^= std::hash<nat>()(tree.at(i)); 
      return result; 
    }; 

  auto equalFun = [] (const std::vector<nat> &treeA,  const std::vector<nat> &treeB)
    {
      bool equal = true;  
      for(nat i = 0; equal && i < treeA.size() ; ++i)
	equal &= treeA.at(i) ==  treeB.at(i); 
      return equal; 
    }; 

  auto trees =  std::unordered_map<std::vector<nat>,nat, decltype(hashFun), decltype(equalFun)>{0,hashFun, equalFun}; 
  for(nat i = 0; i < totalTrees; ++i )
    {
      auto oneTree = std::vector<nat>{}; 
      nat ctr = 0; 
      for(auto &elem : sortedBipOcc)
	{
	  if(elem.second.isSet(i))
	    oneTree.push_back(ctr); 
	  ++ctr; 
	  if(oneTree.size() == numBip)
	    break; 
	}

      assert(oneTree.size() == numBip); 

      auto& iter = trees[oneTree]; 
      ++iter; 
    }

  nat sum = 0; 
  for(auto &elem : trees)
    sum += elem.second; 
  assert(sum == totalTrees); 

  auto treeAndOcc = std::vector<std::pair<std::vector<nat>,nat> >{}; 
  for(auto elem : trees )
    treeAndOcc.push_back(elem); 
  std::sort(treeAndOcc.begin(), treeAndOcc.end(), [](const std::pair<std::vector<nat>,nat> &elemA,
						     const std::pair<std::vector<nat>,nat> &elemB )
	    {
	      return elemA.second > elemB.second; 
	    }); 

  
  auto treeStrings = std::vector<std::pair<std::string, nat> >{}; 
  for(auto &elem : treeAndOcc)
    {
      auto& tree = elem.first;  

      auto bips = std::vector<Bipartition>{}; 
      for(auto &id : tree)
	bips.push_back(sortedBipOcc[id].first); 

      auto result = bipEx.bipartitionsToTreeString(bips, false ); 
      treeStrings.push_back(make_pair(result, elem.second)); 
    }

  nat absThreshold = nat(totalTrees * thresh); 
  std::sort(treeStrings.begin(), treeStrings.end(), [](const std::pair<std::string,nat> &elemA, const std::pair<std::string, nat> &elemB)
	    {
	      return elemA.second > elemB.second; 
	    }); 
  
  std::ofstream outfile(filename); 
  outfile << "freq\ttree" << std::endl; 
  
  nat accProb = 0; 
  for(auto t : treeStrings)
    {
      outfile << t.second << "\t" << t.first << std::endl; 
      accProb += t.second; 
      if(accProb > absThreshold)
	break; 
    }

  std::cout << "printed the " << thresh *  100  <<  "% credible set to " << filename << std::endl; 
} 
