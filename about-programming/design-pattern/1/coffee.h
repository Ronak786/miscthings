#ifndef COFFEE_H_
#define COFFEE_H_

#include <iostream>
#include <string>

namespace drink {
class Beverage {
    private:
        double base_item_price_;
        double size_ = 0;
    public:
        Beverage(double base = 10);
        virtual std::string GetDescription() = 0;  //virtual
        virtual double Cost();
        double GetSize() { return size_; }
        virtual void SetSize(double sz) { 
            size_ = sz;
        }
};

class HouseBlend : public Beverage {
    private:
        double drink_price_;
        std::string desc;
    public:
        HouseBlend(double drink, double base);
        std::string GetDescription() { return "HouseBlend"; }
        double Cost();
};

class DarkRoast : public Beverage {
    private:
        double drink_price_;
        std::string desc;
    public:
        DarkRoast(double drink, double base);
        std::string GetDescription() { return "DarkRoast"; }
        double Cost();
};

// just need interface, so constructor no needed
class CondimentDecorator : public Beverage {
//        std::string GetDescription() = 0;  //virtual
};

class Milk : public CondimentDecorator {
    private:
        Beverage *bptr_;
        double milk_price_;
        std::string desc;
    public:
        Milk(double price, Beverage *base);
        double Cost() { return milk_price_ + bptr_->Cost(); }
        std::string GetDescription() { return "milk"; }
};
        
class Moncha : public CondimentDecorator {
    private:
        Beverage *bptr_;
        double moncha_price_;
        std::string desc;
    public:
        Moncha(double price, Beverage *base);
        double Cost() { return moncha_price_ + bptr_->Cost(); }
        std::string GetDescription() { return "moncha"; }
};

class Cup : public CondimentDecorator {
    private:
        Beverage *bptr_;
    public:
        Cup(Beverage *base) { bptr_ = base; }
        void SetSize(double size) {
            bptr_->SetSize(size);
        }
        std::string GetDescription() { return "Cup"; }
        double Cost() { return bptr_->GetSize() + bptr_->Cost(); }
};
}  //namespace drink

#endif
