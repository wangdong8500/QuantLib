/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2013 Peter Caspers

 This file is part of QuantLib, a free-software/open-source library
 for financial quantitative analysts and developers - http://quantlib.org/

 QuantLib is free software: you can redistribute it and/or modify it
 under the terms of the QuantLib license.  You should have received a
 copy of the license along with this program; if not, please email
 <quantlib-dev@lists.sf.net>. The license is also available online at
 <http://quantlib.org/license.shtml>.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the license for more details.
*/

#include <ql/experimental/models/smilesectionutils.hpp>

namespace QuantLib {

    SmileSectionUtils::SmileSectionUtils(const SmileSection& section,
                                         const std::vector<Real>& moneynessGrid,
                                         const Real atm)  {

        if(moneynessGrid.size()!=0) {
            QL_REQUIRE(moneynessGrid[0] >= 0.0, "moneyness grid should only containt non negative values (" <<
                       moneynessGrid[0] << ")");
            for(Size i=0;i<moneynessGrid.size()-1;i++) {
                QL_REQUIRE(moneynessGrid[i] < moneynessGrid[i+1],
                           "moneyness grid should containt strictly increasing values (" << moneynessGrid[i] << "," <<
                           moneynessGrid[i+1] << " at indices " << i << ", " << i+1 << ")");
            }
        }

        if(atm==Null<Real>()) {
            f_ = section.atmLevel();
            QL_REQUIRE(f_ != Null<Real>(), "atm level must be provided by source section or given in the constructor");
        }
        else {
            f_ = atm;
        }

        std::vector<Real> tmp;

        static const Real defaultMoney[] = {0.0,0.01,0.05,0.10,0.25,0.40,0.50,0.60,0.70,0.80,0.90,
                 1.0,1.25,1.5,1.75,2.0,5.0,7.5,10.0,15.0,20.0};

        if(moneynessGrid.size()==0) tmp = std::vector<Real>(defaultMoney, defaultMoney+21);
        else tmp = std::vector<Real>(moneynessGrid);

        if(tmp[0] > QL_EPSILON) {
            m_.push_back(0.0);
            k_.push_back(0.0);
        }

        for(Size i=0;i<tmp.size();i++) {
            if(fabs(tmp[i]) < QL_EPSILON ||
               (tmp[i]*f_ >= section.minStrike() && tmp[i]*f_ <= section.maxStrike()))
                m_.push_back( tmp[i] );
                k_.push_back( tmp[i] * f_ );
        }

        c_.push_back(f_);

        for(Size i=1;i<k_.size();i++) {
            c_.push_back(section.optionPrice(k_[i],Option::Call,1.0) );
        }

        Size centralIndex =
            std::upper_bound(m_.begin(), m_.end(), 1.0-QL_EPSILON) - m_.begin();
        QL_REQUIRE(centralIndex < k_.size()-1 && centralIndex > 1,
                   "Atm point in moneyness grid (" << centralIndex << ") too close to boundary.");
        leftIndex_ = centralIndex;
        rightIndex_ = centralIndex;

        bool isAf=true;
        do {
            rightIndex_++;
            isAf = af(leftIndex_,rightIndex_,rightIndex_) && af(leftIndex_,rightIndex_-1,rightIndex_);
        } while(isAf && rightIndex_<k_.size()-1);
        if(!isAf) rightIndex_--;

        do {
            leftIndex_--;
            isAf = af(leftIndex_,leftIndex_,rightIndex_) && af(leftIndex_,leftIndex_+1,rightIndex_);
        } while(isAf && leftIndex_>1);
        if(!isAf) leftIndex_++;

        if(rightIndex_ < leftIndex_)
            rightIndex_ = leftIndex_;

    }


    const std::pair<Real,Real> SmileSectionUtils::arbitragefreeRegion() const {
        return std::pair<Real,Real>(k_[leftIndex_],k_[rightIndex_]);
    }

    const std::pair<Size,Size> SmileSectionUtils::arbitragefreeIndices() const {
        return std::pair<Size,Size>(leftIndex_,rightIndex_);
    }

    bool SmileSectionUtils::af(const Size i0, const Size i, const Size i1) const {
        if(i==0) return true;
        Size im = i-1 >= i0 ? i-1 : 0;
        Real q1 = (c_[i] - c_[im]) / (k_[i] - k_[im]);
        if(q1 < -1.0 || q1 > 0.0) return false;
        if(i >= i1) return true;
        Real q2 = (c_[i+1] - c_[i]) / (k_[i+1] - k_[i]);
        if(q1 <= q2) return true;
        return false;
    }


}
