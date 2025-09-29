#include <Rcpp.h>
#include <vector>
#include <algorithm>
using namespace Rcpp;

// Hilfsfunktion: prüft, ob v j enthält oder Marker -1 (alle erlaubt)
bool contains_or_free(const std::vector<int>& v, int j){
  return !v.empty() && (v[0]==-1 || std::find(v.begin(), v.end(), j)!=v.end());
}

// [[Rcpp::export]]
int count_colliding_particles_from_solutions(
    List sols_x, // List der Listen: sols_x[[i]][[k]] = IntegerVector
    List sols_y,
    List sols_z
){
  int N = sols_x.size();
  std::vector<bool> collided(N,false);
  
  for(int i=0;i<N;i++){
    for(int k=i+1;k<N;k++){
      IntegerVector sx = sols_x[i][k];
      IntegerVector sy = sols_y[i][k];
      IntegerVector sz = sols_z[i][k];
      
      std::vector<int> vx(sx.begin(), sx.end());
      std::vector<int> vy(sy.begin(), sy.end());
      std::vector<int> vz(sz.begin(), sz.end());
      
      bool collision=false;
      // Prüfen, ob es eine gemeinsame Zeit gibt
      std::vector<std::vector<int>> sols = {vx, vy, vz};
      for(int d=0;d<3 && !collision;d++){
        for(int j : sols[d]){
          if(j<0) continue;
          bool ok=true;
          for(int dd=0;dd<3;dd++)
            if(!contains_or_free(sols[dd],j)){ok=false; break;}
            if(ok){collision=true; break;}
        }
      }
      
      if(collision){collided[i]=true; collided[k]=true;}
    }
  }
  
  return std::count(collided.begin(), collided.end(), true);
}
