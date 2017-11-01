#ifndef WHETHER_H_
#define WHETHER_H_

#include <iostream>
#include <string>

namespace webdata {
class Subject {
    public:
        virtual void RegisterObserver() = 0;
        virtual void RemoveObserver() = 0;
        virtual void NotifyObserver() = 0;
};

class WebData : public Subject{
    private:
        double one_;
        double two_;
        double three_;
        list<Observer*> oblist_;
    public:
        WebData(double a, double b, double c);
        void SetData(double a, double b, double c) {
            one_ = a;
            two_ = b;
            three_ = c;
        }
        void RegisterObserver(const Observer* obptr); 
        void RemoveObserver(const Observer* obptr); 
        void NotifyObserver(); 
};

class Observer {
    private:
        Subject *subptr_ = nullptr;
    public:
        void Subscribe(Subject *subptr);
        virtual void update() = 0;
        virtual void display() = 0;
};

class CurrentConditionDisplay : public Observer {
    public:
        void update();
        void display();
};

class StatisticsDisplay : public Observer {
    public:
        void update();
        void display();
};

class ForecastDisplay : public Observer {
    public:
        void update();
        void display();
};

class ThirdPartyDisplay : public Observer {};
} //namespace webdata
