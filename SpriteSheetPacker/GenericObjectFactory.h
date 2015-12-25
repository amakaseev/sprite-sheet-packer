//
//  GenericObjectFactory.h
//  RunAndJump
//
//  Created by Aleksey Makaseev on 9/11/15.
//
//

#ifndef __GameObjectFactory__
#define __GameObjectFactory__

template <class ID, class Base, class ... Args>
class GenericObjectFactory {
private:
    typedef Base* (*fInstantiator)(Args ...);
    template <class Derived> static Base* instantiator(Args ... args) {
        return new Derived(args ...);
    }
    std::map<ID, fInstantiator> _classes;
    
public:
    GenericObjectFactory() {
        
    }
    
    template <class Derived> void set(ID id) {
        _classes[id] = &instantiator<Derived>;
    }
    fInstantiator get(ID id) {
        return _classes[id];
    }
};

#endif /* defined(__GameObjectFactory__) */
