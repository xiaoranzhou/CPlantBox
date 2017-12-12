#include "Organ.h"

/**
 * Constructor
 */
Organ::Organ(Plant* plant, Organ* parent, int subtype, double delay) :plant(plant), parent(parent), age(delay)
{
  id = plant->getOrganIndex(); // unique id from the plant
}

/**
 * Destructor, tell the kids (bad news)
 */
Organ::~Organ()
{
  for(auto c : children) {
      delete c;
  }
  delete param;
}

/**
 * The organ origin in absolute coordinates
 * p_abs = A0(p1+A1(p2+A2(p3+... A(n-1)(pn ) )))), where A are the relative headings, p are the relative origins
 */
Vector3d Organ::getOrigin() const {
  // recursive
  if (this->organType() != Organ::ot_seed) {
      return parent->getOrigin()+parent->getHeading().times(getRelativeOrigin());
  } else {
      return this->getRelativeOrigin()();
  }
/*// sequentiell
  const Organ* o = this;
  Vector3d p = o->getRelativeOrigin();
  o = o->parent;
  while (o->organType() != Organ::ot_seed) {
      Vector3d rp = o->getRelativeOrigin();
      Matrix3d A = o->getRelativeHeading();
      p = rp.plus(A.times(p));
      o = o->parent;
  }
  p.plus(o->getRelativeOrigin());
  return p; */
}

/**
 * Absolute organ heading
 * Hn =(A0*A1*..An), where H is the absolute Heading, and A are the relative headings
 */
Matrix3d Organ::getHeading() const {
  // recursive
  if (this->organType() != Organ::ot_seed) {
      auto a = parent->getHeading();
      a.times(getRelativeHeading());
      return a;
  } else {
      return this->getRelativeOrigin()();
  }
/*// sequentiell
  const Organ* o = this;
  Matrix3d ah = Matrix3d();
  while (o->organType() != Organ::ot_seed) {
      Matrix3d iH = o->getRelativeHeading();
      iH.times(ah);
      ah = iH;
      o = o->parent;
  }
  return ah; */
}

/**
 * Computes the absolute node coordinate (A*x+o)
 */
Vector3d Organ::getNode(int i) const
{
  return (getHeading().times(r_nodes.at(i))).plus(getOrigin());
}

/**
 * Computes the absolute node coordinates for all relative coordinates (A*x_i+o)
 */
std::vector<Vector3d> Organ::getNodes() const
{
  std::vector<Vector3d> nodes(r_nodes.size());
  auto A = getHeading();
  auto p0 = getOrigin();
  for (size_t i=0; i<r_nodes.size(); i++) {
      nodes.at(i) = A.times(r_nodes.at(i)).plus(p0);
  }
  return nodes;
}

/**
 * Asks the plant for the organ type parameter
 */
OrganTypeParameter* Organ::getOrganTypeParameter() const
{
  return plant->getParameter(this->organType(), param->subType);
}

/**
 * Calls suborgans (children)
 */
void Organ::simulate(double dt, bool silence)
{
  age+=dt;

  if (age>0) {
      for (auto& c : children)  {
          c->simulate(dt);
      }
  }
}

/**
 * todo
 */
double Organ::getScalar(int stype) const {
	switch(stype) {
	case Plant::st_one:
		return 1;
	case Plant::st_id:
		return id;
	case Plant::st_otype:
		return this->organType();
	case Plant::st_subtype:

		//check the type of the organ
		//    std::cout<<"organtype "<<organType()<<" , subtype "<<param->subType<<std::endl;
		//used to debug and check organType and reference
         return param->subType;


		case Plant::st_alive:
			return alive;
		case Plant::st_active:
			return active;
		case Plant::st_age:
			return age;
		case Plant::st_length:
			return length;
		case Plant::st_order: {
			int c=0;
			const Organ* p = this;
			while (p->organType()!=Organ::ot_seed) {
				c++;
				p = p->parent; // up organ tree
			}
			return c;
		}
		case Plant::st_parenttype:
			if (this->parent!=nullptr) {
				return this->parent->organType();
			} else {
				return std::nan("");
			}
		case Plant::st_time: {
			if (nctimes.size()>0) {
				return nctimes.at(0);
			} else {
				return std::nan("");
			}
		}
		default:
		  return std::nan("");
	}
}

/**
 * Returns the organs as sequential list,
 * copies only organs with more than 1 node.
 *
 * \return sequential list of organs
 */
std::vector<Organ*> Organ::getOrgans(unsigned int otype)
{
  std::vector<Organ*> v = std::vector<Organ*>();
  this->getOrgans(otype, v);
  return v;
}

/**
 * Returns the organs as sequential list,
 * copies only organs with more than 1 node.
 *
 * @param v     adds the organ sub tree to this vector
 */
void Organ::getOrgans(unsigned int otype, std::vector<Organ*>& v)
{
  if (this->r_nodes.size()>1) {
      unsigned int ot = this->organType();
      if ((otype & ot) > 0) {
          v.push_back(this);
      }
  }
  for (auto const& c:this->children) {
      c->getOrgans(otype,v);
  }
}

/**
 * writes RSML root tag (todo update for general organs)
 *
 * @param cout      typically a file out stream
 * @param indent    we care for looks
 */
void Organ::writeRSML(std::ostream & cout, std::string indent) const
{
  if (this->r_nodes.size()>1) {
      cout << indent << "<root id=\"" <<  id << "\">\n";  // open root

      /* geometry tag */
      cout << indent << "\t<geometry>\n"; // open geometry
      cout << indent << "\t\t<polyline>\n"; // open polyline
      // polyline nodes
      cout << indent << "\t\t\t" << "<point ";
      Vector3d v = this->getNode(0);
      cout << "x=\"" << v.x << "\" y=\"" << v.y << "\" z=\"" << v.z << "\"/>\n";
      int n = 1; //this->plant->rsmlReduction;
      for (size_t i = 1; i<r_nodes.size()-1; i+=n) {
          cout << indent << "\t\t\t" << "<point ";
          Vector3d v = this->getNode(i);
          cout << "x=\"" << v.x << "\" y=\"" << v.y << "\" z=\"" << v.z << "\"/>\n";
      }
      cout << indent << "\t\t\t" << "<point ";
      v = r_nodes.back();
      cout << "x=\"" << v.x << "\" y=\"" << v.y << "\" z=\"" << v.z << "\"/>\n";
      cout << indent << "\t\t</polyline>\n"; // close polyline
      cout << indent << "\t</geometry>\n"; // close geometry

      /* properties */
      cout << indent <<"\t<properties>\n"; // open properties
      // TODO
      cout << indent << "\t</properties>\n"; // close properties

      cout << indent << "\t<functions>\n"; // open functions
      cout << indent << "\t\t<function name='emergence_time' domain='polyline'>\n"; // open functions
      cout << indent << "\t\t\t" << "<sample>" << nctimes[0] << "</sample>\n";
      for (size_t i = 1; i<nctimes.size()-1; i+=n) {
          cout << indent << "\t\t\t" << "<sample>" << nctimes.at(i) << "</sample>\n";

      }
      cout << indent << "\t\t\t" << "<sample>" << nctimes.back() << "</sample>\n";

      cout << indent << "\t\t</function>\n"; // close functions
      cout << indent << "\t</functions>\n"; // close functions

      /* laterals roots */
      for (size_t i = 0; i<children.size(); i++) {
          children[i]->writeRSML(cout,indent+"\t");
      }

      cout << indent << "</root>\n"; // close root
  }
}

/**
 * Quick info about the object for debugging TODO change param to param or param
 */
std::string Organ::toString() const
{
  std::stringstream str;
  str << "Organ #"<< id <<": type "<< param->subType << ", length: "<< length << ", age: " << age
      <<" with "<< children.size() << " successors\n";
  return str.str();
}


