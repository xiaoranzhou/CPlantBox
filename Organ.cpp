#include "Organ.h"

/**
 * Constructor
 */
Organ::Organ(Plant* plant, Organ* parent, int subtype, double delay) :plant(plant), parent(parent), age(delay)
{
	id = plant->getOrganIndex();
}

/**
 * Destructor, tell the kids
 */
Organ::~Organ()
{
	for(auto c : children) {
		delete c;
	}
	delete stem_param;
	delete root_param;
}


/**
 * todo
 1. get declaration of the parameters and
 2. future can use cast override to increase efficiency
 */
OrganTypeParameter* Organ::getOrganTypeParameter() const
{
	switch (organType()) {//check the type of the organ
	//    std::cout<<"organtype "<<organType()<<" , subtype "<<stem_param<<std::endl;
	//used to debug and check organType and reference

	case Organ::ot_seed :return plant->getOrganTypeParameter(this->organType(), stem_param->subType);
	case Organ::ot_root :return plant->getOrganTypeParameter(this->organType(), root_param->subType);
	case Organ::ot_stem :return plant->getOrganTypeParameter(this->organType(), stem_param->subType);
	case Organ::ot_leafe :return plant->getOrganTypeParameter(this->organType(), leaf_param->subType);
	case Organ::ot_shoot :return plant->getOrganTypeParameter(this->organType(), stem_param->subType);



	}
};

/**
 * todo test...
 */
Vector3d Organ::getAbsoluteOrigin() const {
	const Organ* p = this;
	Vector3d o0 = Vector3d();
	while (p->organType() != Organ::ot_seed) {
		Vector3d o;
		if (p->parent!=0) {
			o = parent->getRelativeInitialHeading().times(p->getRelativeOrigin());
		} else {
			o = p->getRelativeOrigin();
		}
		o0.plus(o);
		p = p->parent;
	}
	return o0;
}

/**
 * todo test, multiplication from left?
 */
Matrix3d Organ::getAbsoluteInitialHeading() const {
	const Organ* p = this;
	Matrix3d ah = Matrix3d();
	while (p->organType() != Organ::ot_seed) {
		Matrix3d iH = p->getRelativeInitialHeading();
		iH.times(ah);
		ah = iH;
		p = p->parent;
	}
	return ah;
}

/**
 *
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

		switch (organType()) {//check the type of the organ
		//    std::cout<<"organtype "<<organType()<<" , subtype "<<stem_param->subType<<std::endl;
		//used to debug and check organType and reference
		case Organ::ot_seed :return stem_param->subType;
		case Organ::ot_root :return root_param->subType;
		case Organ::ot_stem :return stem_param->subType;
		case Organ::ot_leafe :return leaf_param->subType;
		case Organ::ot_shoot :return stem_param->subType;

		}
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
		switch(otype) {
		case Organ::ot_organ:
			v.push_back(this);
		break;
		case Organ::ot_seed:
			if (ot==Organ::ot_seed) {
				v.push_back(this);
			}
			break;
		case Organ::ot_root:
			if (ot==Organ::ot_root) {
				v.push_back(this);
			}
			break;
		case Organ::ot_stem:
			if (ot==Organ::ot_stem) {
				v.push_back(this);
			}
			break;
		case Organ::ot_leafe:
			if (ot==Organ::ot_leafe) {
				v.push_back(this);
			}
			break;
		case Organ::ot_shoot:
			if ((ot==Organ::ot_leafe)||(ot==Organ::ot_stem)) {
				v.push_back(this);
			}
		break;
		default:
			throw std::invalid_argument( "Organ::getOrgans: unknown organ type" );
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
 * Quick info about the object for debugging TODO change param to stem_param or root_param
 */
std::string Organ::toString() const
{
	std::stringstream str;
	str << "Organ #"<< id <<": type "<< root_param->subType << ", length: "<< length << ", age: " << age
			<<" with "<< children.size() << " successors\n";
	return str.str();
}

///Organ Type Parameter read function


