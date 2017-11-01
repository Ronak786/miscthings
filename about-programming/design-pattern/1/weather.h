#ifndef WHETHER_H_
#define WHETHER_H_

#include <iostream>
#include <list>
#include <string>

namespace webdata {
class Observer;
class Subject {
    public:
        virtual void RegisterObserver(Observer*) = 0;
        virtual void RemoveObserver(Observer*) = 0;
        virtual void NotifyObserver() = 0;
};

class WebData : public Subject{
    public:
        WebData(double a, double b, double c);
        void SetData(double a, double b, double c) {
            one_ = a;
            two_ = b;
            three_ = c;
        }
        void RegisterObserver(Observer* obptr); 
        void RemoveObserver(Observer* obptr); 
        void NotifyObserver(); 
    private:
        double one_;
        double two_;
        double three_;
        std::list<Observer*> oblist_;
};

class Observer {
    public:
        void Subscribe(Subject *subptr);
        virtual void update() = 0;
        virtual void display() = 0;
    private:
        Subject *subptr_ = nullptr;
};

class CurrentConditionDisplay : public Observer {
    public:
        void update() { std::cout << "in Current update" << std::endl; }
        void display() { std::cout << "in curr display" << std::endl; }
};

class StatisticsDisplay : public Observer {
    public:
        void update() { std::cout << "in Statistics update" << std::endl; }
        void display() { std::cout << "in stat display" << std::endl; }
};

class ForecastDisplay : public Observer {
    public:
        void update() { std::cout << "in Forecase update" << std::endl; }
        void display() { std::cout << "in fore display" << std::endl; }
};

class ThirdPartyDisplay : public Observer {};
} //namespace webdata

#endif
