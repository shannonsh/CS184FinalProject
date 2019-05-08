#include "material.h"

using namespace std;

namespace CGL {

  void Material::copy(Material* source) {
    this->id =   source->id;
    this->name = source->name;
    this->type = source->type;
    this->emit = source->emit;
    this->ambi = source->ambi;
    this->diff = source->diff;
    this->spec = source->spec;
    this->shininess = source->shininess;
    this->refractive_index = source->refractive_index;
  }
  
  std::ostream& operator<<( std::ostream& os, const Material& material ) {

    os << "Material: " << material.name << " (id:" << material.id << ")";


    os << " [";

    os << " ambi=(" << material.ambi.r << ","
    << material.ambi.g << ","
    << material.ambi.b << ","
    << material.ambi.a << ")";
    os << " diff=(" << material.diff.r << ","
    << material.diff.g << ","
    << material.diff.b << ","
    << material.diff.a << ")";
    os << " spec=(" << material.spec.r << ","
    << material.spec.g << ","
    << material.spec.b << ","
    << material.spec.a << ")";

    os << " shininess=" << material.shininess;

    os << " refractive_index=" << material.refractive_index;

    os << " ]";

    return os;
  }

} // namespace CGL
