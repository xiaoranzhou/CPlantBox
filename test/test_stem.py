import unittest
import sys
sys.path.append("..")
import plantbox as pb
import numpy as np
from rb_tools import *


def stemAge(l, r, k):  # stem age at a certain length
    return -np.log(1 - l / k) * k / r


def stemLength(t, r, k):  # stem length at a certain age
    return k * (1 - np.exp(-r * t / k))


def stemLateralLength(t, et, r, k):  # length of first order laterals (without second order laterals)
    i, l = 0, 0
    while et[i] < t:
        age = t - et[i]
        l += stemLength(age, r, k)
        i += 1
    return l


class TestStem(unittest.TestCase):

    def stem_example_rtp(self):
        """ an example used in the tests below, a main stem with laterals """
        self.plant = pb.Organism()  # Stem has no dependency on StemSystem anymore
        p0 = pb.StemRandomParameter(self.plant)
        p0.name, p0.type, p0.la, p0.lb, p0.nob, p0.ln, p0.r, p0.dx = "mainstem", 1, 1, 10, 20, (89. / 19.), 1, 0.5
        p0.successor = a2i([2])  # to pb.std_int_double_()
        p0.successorP = a2v([1.])  # pb.std_vector_double_()
        #print(p0)
        p1 = pb.StemRandomParameter(self.plant)
        #print(p1)
        p1.name, p1.type, p1.la, p1.ln, p1.r, p1.dx = "lateral", 2, 25, 0, 2, 0.1
        #print(p1)
        self.p0, self.p1 = p0, p1  # Python will garbage collect them away, if not stored
        self.plant.setOrganRandomParameter(self.p0)  # the organism manages the type parameters
        self.plant.setOrganRandomParameter(self.p1)
        self.param0 = self.p0.realize()  # set up stem by hand (without a stem system)
        self.param0.la = 0  # its important parent has zero length, otherwise creation times are messed up
        self.param0.lb = 0
        # param0 is stored, because otherwise garbage collection deletes it, an program will crash <---
        parentstem = pb.Stem(3, self.param0, True, True, 0., 0., pb.Vector3d(0, 0, -1), 0, 0, False, 0)
        parentstem.setOrganism(self.plant)
        parentstem.addNode(pb.Vector3d(0, 0, -3), 0)  # there is no nullptr in Python
        self.stem = pb.Stem(self.plant, self.p0.subType, pb.Vector3d(0, 0, -1), 0, parentstem, 0, 0)
        self.stem.setOrganism(self.plant)
        print("rtp_finsih")

    def stem_length_test(self, dt, l, subDt):
        print(p1 ,"start")
        """ simulates a single stem and checks length against analytic length """
        nl, nl2, non, meanDX = [], [], [], []

        for t in dt:
            for i in range(0, subDt):
                
                self.stem.simulate(t / subDt)
                nl.append(self.stem.getParameter("length"))
                non.append(self.stem.getNumberOfNodes())
                meanDX.append(nl[-1] / non[-1])
                print("finsih")

            # length from geometry
            poly = np.zeros((non[-1], 3))  #
            for i in range(0, non[-1]):
                v = self.stem.getNode(i)
                poly[i, 0] = v.x
                poly[i, 1] = v.y
                poly[i, 2] = v.z
            d = np.diff(poly, axis = 0)
            sd = np.sqrt((d ** 2).sum(axis = 1))
            nl2.append(sum(sd))
        for i in range(0, len(dt)):
            self.assertAlmostEqual(l[i], nl[i], 10, "numeric and analytic lengths do not agree in time step " + str(i + 1))
            self.assertAlmostEqual(l[i], nl2[i], 10, "numeric and analytic lengths do not agree in time step " + str(i + 1))
            self.assertLessEqual(meanDX[i], 0.5, "axial resolution dx is too large")
            self.assertLessEqual(0.25, meanDX[i], "axial resolution dx is unexpected small")
        print(p1 ,"finsih")

    def test_constructors(self):
        print("start_constructor")
        """ tests three different kinds of constructors """
        self.stem_example_rtp()
        # 1. constructor from scratch
        print("befor_realize")
        param = self.p0.realize()
        print("befor_realize2")
        stem = pb.Stem(3, param, True, True, 0., 0., pb.Vector3d(0, 0, -1), 0, 0, False, 0)
        print("befor_realize3")
        stem.setOrganism(self.plant)
        stem.addNode(pb.Vector3d(0, 0, -3), 0)  # parent must have at least one nodes
        # 2. used in simulation (must have parent, since there is no nullptr in Pyhton)
        stem2 = pb.Stem(self.plant, self.p1.subType, pb.Vector3d(0, 0, -1), 0, stem, 0, 0)
        stem.addChild(stem2);
        # 3. deep copy (with a factory function)
        plant2 = pb.Organism()
        stem3 = stem.copy(plant2)
        self.assertEqual(str(stem), str(stem3), "deep copy: the organs shold be equal")
        self.assertIsNot(stem.getParam(), stem3.getParam(), "deep copy: organs have same parameter set")
        # TODO check if OTP were copied
        print(p1 ,"finsih")

    def test_stem_length(self):
        print("start_length")
        """ tests if numerical stem length agrees with analytic solutions at 4 points in time with two scales of dt"""
        self.stem_example_rtp()
        times = np.array([0., 7., 15., 30., 60.])
        dt = np.diff(times)
        k = self.stem.param().getK()  # maximal stem length
        self.assertAlmostEqual(k, 100, 12, "example stem has wrong maximal length")
        l = stemLength(times[1:], self.p0.r, k)  # analytical stem length
        stem = self.stem.copy(self.plant)
        self.stem_length_test(dt, l, 1)  # large dt
        self.stem = stem
        self.stem_length_test(dt, l, 1000)  # very fine dt
        print(p1 ,"finsih")

    def test_stem_length_including_laterals(self):
        print("start_laterals")
        """ tests if numerical stem length agrees with analytic solution including laterals """
        self.stem_example_rtp()
        times = np.array([0., 7., 15., 30., 60.])
        dt = np.diff(times)
        p = self.stem.param()  # rename
        k = p.getK()
        et = np.zeros((p.nob))
        l = 0
        et[0] = stemAge(p.la + p.lb + l, p.r, k)
        for i in range(0, p.nob - 1):  # calculate lateral emergence times
            l += p.ln[i]
            et[i + 1] = stemAge(p.la + p.lb + l, p.r, k + 1e-12)
        l = stemLength(times[1:], p.r, k)  # zero order lengths
        l1 = []
        r2 = self.p1.r
        k2 = self.p1.la  # consists of lateral zone only
        for t in times[1:]:
            l1.append(stemLateralLength(t, et, r2, k2))
        analytic_total = l + l1

        for subDX in [1, 1000]:
            numeric_total = []
            for t in times[1:]:
                stem = self.stem.copy(self.plant)
                self.stem_length_test([t], [stemLength(t, p.r, k)], subDX)
                organs = self.stem.getOrgans()
                nl = 0
                for o in organs:
                    nl += o.getParameter("length")
                numeric_total.append(nl);
                self.stem = stem
            for i in range(0, len(times[1:])):
                self.assertAlmostEqual(numeric_total[i], analytic_total[i], 10, "numeric and analytic total lengths do not agree in time step " + str(i + 1))
        print(p1 ,"finsih")

    def test_geometry(self):
        print("start_geometry")
        """ tests if nodes can be retrieved from the organ """
        # TODO make plot for plausibility

    def test_parameter(self):
        print(p1 ,"start")
        """ tests some parameters on sequential organ list """
        self.stem_example_rtp()
        self.stem.simulate(30)
        organs = self.stem.getOrgans()
        type, age, radius, order, ct = [], [], [], [], []
        for o in organs:
            type.append(o.getParameter("subType"))
            age.append(o.getParameter("age"))
            ct.append(o.getParameter("creationTime"))
            radius.append(o.getParameter("radius"))
            order.append(o.getParameter("order"))
        self.assertEqual(type, [1.0, 2.0, 2.0, 2.0, 2.0], "getParameter: unexpected stem sub types")
        self.assertEqual(order, [1.0, 2.0, 2.0, 2.0, 2.0], "getParameter: unexpected stem sub types")  # +1, because of artificial parent stem
        for i in range(0, 5):
            self.assertEqual(age[i], 30 - ct[i], "getParameter: unexpected stem sub types")  # +1, because of artificial parent stem
        print(p1 ,"finsih")
#

    def test_dynamics(self):
        print("start_dynamics")
        """ tests if nodes created in last time step are correct """  #
        self.stem_example_rtp()
        r = self.stem
        print("start_dynamics2")
        r.simulate(.5, True)
        
        self.assertEqual(r.hasMoved(), False, "dynamics: node movement during first step")
        r.simulate(1e-1, True)
        self.assertEqual(r.hasMoved(), False, "dynamics: movement, but previous node at axial resolution")
        r.simulate(1e-1, True)
        self.assertEqual(r.hasMoved(), True, "dynamics: node was expected to move, but did not")
        r.simulate(2.4, True)
        self.assertEqual(r.getNumberOfNodes() - r.getOldNumberOfNodes(), 5, "dynamics: unexcpected number of new nodes")
        print(p1 ,"finsih")


if __name__ == '__main__':
    unittest.main()