#include "Seed.h"

Seed::Seed(Plant* plant) :Organ(plant, nullptr, 0, 0), seed_pos(Vector3d(0,0,0))
{
	param = (SeedParameter*)plant->getParameter(Organ::ot_seed,0)->realize();
}

SeedParameter* Seed::initializeparam()
{
	// Create root system

	SeedTypeParameter* stp = (SeedTypeParameter*)plant->getParameter(Organ::ot_seed, 0);
	param = stp->realize(); // throw the dice
	SeedParameter* sparam = (SeedParameter*)param;
	std::cout << "maxb = " << sparam->maxB << std::endl;
	return sparam;
}



/**
 *
 */
void Seed::initialize(SeedParameter* sparam)
{
	const double maxT = 365.; // maximal simulation time
	Vector3d iheading(0,0,-1);
	if (Plant::noParamFile[1] == 1) {
		std::cout<<"no root param"<<std::endl;
	} else {
		Root* taproot = new Root(plant, this, 1, 0, iheading ,0., 0.); // tap root has subtype 1
		taproot->addNode(sparam->seedPos,0);
		std::cout << "sparam->seedPos is" << sparam->seedPos.toString() << "\n" ;
		children.push_back(taproot);
		if (sparam->maxB>0) {
			if (plant->getParameter(Organ::ot_root, basalType)->subType<1) { // if the type is not defined, copy tap root
				std::cout << "Basal root type #" << basalType << " was not defined, using tap root parameters instead\n";
				RootTypeParameter* tapParam = (RootTypeParameter*)plant->getParameter(Organ::ot_root, 1);
				RootTypeParameter* brtp = new RootTypeParameter(*tapParam);
				brtp->subType = basalType;
				plant->setParameter(brtp);
				std::cout << "maxb = " << sparam->maxB << std::endl;

			}
			int maxB = sparam->maxB;
			if (sparam->delayB>0) {
				maxB = std::min(maxB,int(std::ceil((maxT-sparam->firstB)/sparam->delayB))); // maximal for simtime maxT
			}
			double delay = sparam->firstB;
			for (int i=0; i<maxB; i++) {
				Root* basalroot = new Root(plant, this, 1, delay, iheading ,0., 0.);
				basalroot->r_nodes.push_back(taproot->r_nodes.at(0)); // node
				basalroot->nodeIDs.push_back(taproot->nodeIDs.at(0)); // tap root ID
				basalroot->nctimes.push_back(delay); // exact creation time
				children.push_back(basalroot);
				delay += sparam->delayB;
			}
		}
	}


	//main stem initialation

	if (Plant::noParamFile[2] == 1) {
		std::cout<<"no stem parameter, maybe the XML based parameter file is directly converted from the old rparam file"<<std::endl;
	} else {
		//	Vector3d isheading(0,0,1);//Initial Stem heading
		Vector3d isheading(0, 0, 1);//Initial Stem heading
		Stem* mainstem = new Stem(plant, this, 1, 0., isheading, 0., 0.); // tap root has subtype 1
		mainstem->addNode(sparam->seedPos, 0);
		children.push_back(mainstem);

		//	Stem* tiller1 = new Stem(plant, this, 1, 2, isheading ,0., 0.); // tap root has subtype 1
		//	tiller1->addNode(sparam->seedPos,0);
		//	children.push_back(tiller1);
		//
		//	Stem* tiller2 = new Stem(plant, this, 1, 4 , isheading ,0., 0.); // tap root has subtype 1
		//	tiller2->addNode(sparam->seedPos,0);
		//	children.push_back(tiller2);
		//	Stem* tiller3 = new Stem(plant, this, 1, 5, isheading ,0., 0.); // tap root has subtype 1
		//	tiller3->addNode(sparam->seedPos,0);
		//	children.push_back(tiller3);
	}

}

/**
 * Quick info about the object for debugging
 */
std::string Seed::toString() const
{
	std::stringstream str;
	str << "Seed #"<< id <<": type "<< param->subType << ", length: "<< length << ", age: " << age;
	return str.str();
}