#ifndef _VECTOR_H_
#define _VECTOR_H_

#include <cstdint>
#include <stdexcept>
#include <utility>

//Utility gives std::rel_ops which will fill in relational
//iterator operations so long as you provide the
//operators discussed in class.  In any case, ensure that
//all operations listed in this website are legal for your
//iterators:
//http://www.cplusplus.com/reference/iterator/RandomAccessIterator/
using namespace std::rel_ops;

namespace epl{

    class invalid_iterator {
	public:
        enum SeverityLevel {SEVERE,MODERATE,MILD,WARNING};
        SeverityLevel level;

        invalid_iterator(SeverityLevel level = SEVERE){ this->level = level; }
        virtual const char* what() const {
            switch(level){
                case WARNING:   return "Warning"; // not used in Spring 2015
                case MILD:      return "Mild";
                case MODERATE:  return "Moderate";
                case SEVERE:    return "Severe";
                default:        return "ERROR"; // should not be used
            }
        }
    };

    static int min_capacity = 8;
    template <typename T>
    class vector{
    private:
        T* dbegin;
        T* dend;
        T* sbegin;
        T* send;
        uint64_t length;
        uint64_t storage;
        uint64_t front_storage;
        uint64_t reallocate_times;
        uint64_t vector_version;

    public:
        vector(void){
            sbegin = reinterpret_cast<T*> (operator new(sizeof(T) * min_capacity));
            send = sbegin + min_capacity;
            dbegin = dend = sbegin;
            storage = min_capacity;
            length = 0;
            front_storage = 0;
            reallocate_times = 0;
            vector_version = 0;
        }

        explicit vector(uint64_t n){
            if(n == 0)  vector();
            else
            {
                storage = n;
                length = n;
                sbegin = reinterpret_cast<T*> (operator new(sizeof(T) * storage));
                send = sbegin + storage;
                dbegin = sbegin;
                dend = dbegin + length;
                front_storage = 0;
                for(uint64_t k = 0; k < length; k += 1){
                    new (dbegin+k) T();
                }
                reallocate_times = 0;
                vector_version = 0;
            }
        }

        /********************constructor from initializer_list*************************/
        vector(std::initializer_list<T> i1){
            uint64_t storage = i1.size();
            if (storage == 0) { storage = min_capacity; }
            sbegin = reinterpret_cast<T*>(operator new(storage * sizeof(T)));
            send = sbegin + storage;
            dbegin = dend = sbegin;
            for (auto iter = i1.begin(); iter != i1.end(); ++iter){
                new (dend) T(*iter);
                ++dend;
            }
            length = i1.size();
            front_storage = 0;
            reallocate_times = 0;
            vector_version = 0;
        }


         /********************constructor from iterator****************************/
        template<typename IT>
        vector(IT b, IT e){
            typename std::iterator_traits<IT>::iterator_category x{};
            build_vector(b, e, x);
        }

        template<typename IT>
        void build_vector(IT& b, IT& e, std::random_access_iterator_tag x){
            storage = e- b;
            length = e - b;
            sbegin = reinterpret_cast<T*> (operator new(sizeof(T) * storage));
            send = sbegin + storage;
            dbegin = sbegin;
            dend = dbegin + length;
            front_storage = 0; uint64_t k=0;
            while(b != e){
                new (dbegin+k) T(*b); b++; k++;
            }
            reallocate_times = 0;
            vector_version = 0;
        }

        template<typename IT, typename TAG>
        void build_vector(IT& b, IT& e, TAG x){
            storage = min_capacity;
            sbegin = reinterpret_cast<T*> (operator new(sizeof(T) * storage));
            send = sbegin + storage;
            dbegin = sbegin; dend = dbegin;
            front_storage = 0; length = 0;
            while (b != e) {
                push_back(*b); b++; length++;
            }
            reallocate_times = 0;
            vector_version = 0;
        }



        /*********************copy construcot and assignment**************************/
        vector(const vector<T>& that) {
            copy(that);
            reallocate_times = 0;
            vector_version = 0;
        }

        vector<T>& operator=(const vector<T>& that) {
            if(this != &that){
                destroy();
                copy(that);
            }
            reallocate_times++;
            vector_version++;
            return *this;
        }

        /*********move constructor and assigment operator  part b***************************/
        vector(vector<T>&& that )  {
            move(std::move(that));
            reallocate_times = 0;
            vector_version = 0;
        }

        vector<T>& operator=(vector<T>&& that){
            if(this != &that){
                destroy();
                move(std::move(that));
            }
            reallocate_times++;
            vector_version++;
            return *this;
        }

        ~vector(void) { destroy(); }

        uint64_t size(void) const{
            return (dend - dbegin);
        }

        T& operator[](uint64_t k){
            if( (k + dbegin) >= dend) { throw std::out_of_range{"subscript out of range"}; }
            return dbegin[k];
        }

        T& operator[](uint64_t k) const{
            if( (k + dbegin) >= dend) { throw std::out_of_range{"subscript out of range"}; }
            return dbegin[k];
        }

        void push_back(const T& that){
            if(send == dend){
                storage = storage * 2;
                reallocate_times++;
                T* sbegin1 = reinterpret_cast<T*> (operator new(sizeof(T) * storage));
                T *send1 = sbegin1 + storage;
                T* dbegin1 = sbegin1 + front_storage;
                T* dend1 = dbegin1 + length;

                /***********copy before being null*************************/
                //new(dend1) T(that);

                for(uint64_t k = 0; k < length; k++){
                    //new(dbegin1+k) T((dbegin[k]));
                    new(dbegin1+k) T( std::move(dbegin[k]));
                }
                new(dend1) T(that);
                destroy();

                sbegin = sbegin1; dbegin = dbegin1; dend = dend1 + 1 ; send = send1;
                length++;
            }
            else{
                new(dend) T(that);
                dend ++;
                length++;
            }
            vector_version++;
        }

        void push_back(T&& that){
            if(send == dend){
                storage = storage * 2;
                reallocate_times++;
                T* sbegin1 = reinterpret_cast<T*> (operator new(sizeof(T) * storage));
                T *send1 = sbegin1 + storage;
                T* dbegin1 = sbegin1 + front_storage;
                T* dend1 = dbegin1 + length;

                /************move before being null**************************/
                new(dend1) T(std::move(that));

                for(uint64_t k = 0; k < length; k++){
                    //new(dbegin1+k) T((dbegin[k]));
                    new(dbegin1+k) T( std::move(dbegin[k]));
                }
                //new(dend1) T(that);
                destroy();

                sbegin = sbegin1; dbegin = dbegin1; dend = dend1 + 1 ; send = send1;
                length++;
            }
            else{
                //new(dend) T(that);
                new(dend) T(std::move(that));
                dend ++;
                length++;
            }
            vector_version++;
        }

        void push_front(const T& that){
            if(sbegin == dbegin) {
                uint64_t old_storage = storage;
                storage = storage * 2;
                reallocate_times++;
                T* sbegin1 = (T*)(operator new(sizeof(T) * storage));
                T* send1 = sbegin1 + storage;
                T* dbegin1 = sbegin1 + front_storage + old_storage ;
                T* dend1 = dbegin1 + length;

                /***********copy before being null******************************/
                new(dbegin1 -1) T{that};

                for(uint64_t i = 0; i < length; i++){
                    //new(dbegin1 + i) T{dbegin[i]};
                    new(dbegin1 + i) T{std::move(dbegin[i])};
                }

                destroy();

                sbegin = sbegin1; send = send1; dbegin = dbegin1 -1; dend = dend1;
                front_storage = front_storage + old_storage -1;
                length++;
            }
            else{
                dbegin--;
                new(dbegin) T{that};
                length++;
                front_storage--;
            }
            vector_version++;
        }

        void push_front(T&& that){
            if(sbegin == dbegin) {
                uint64_t old_storage = storage;
                storage = storage * 2;
                reallocate_times++;
                T* sbegin1 = (T*)(operator new(sizeof(T) * storage));
                T* send1 = sbegin1 + storage;
                T* dbegin1 = sbegin1 + front_storage + old_storage ;
                T* dend1 = dbegin1 + length;

                /***********move before being null****************************/
                new(dbegin1 -1) T{std::move(that)};

                for(uint64_t i = 0; i < length; i++){
                    //new(dbegin1 + i) T{dbegin[i]};
                    new(dbegin1 + i) T{std::move(dbegin[i])};
                }
                //new(dbegin1 -1) T{that};

                destroy();

                sbegin = sbegin1; send = send1; dbegin = dbegin1 -1; dend = dend1;
                front_storage = front_storage + old_storage -1;
                length++;
            }
            else{
                dbegin--;
                //new(dbegin) T{that};
                new(dbegin) T{std::move(that)};
                length++;
                front_storage--;
            }
            vector_version++;
        }

        void pop_back(void){
            if(dbegin == dend) { throw std::out_of_range{"no data to be poped"}; }
            dend--;
            dend -> ~T();
            length--;
            vector_version++;
        }

        void pop_front(void){
            if(dbegin == dend) { throw std::out_of_range{"no data to be poped"}; }
            dbegin -> ~T();
            dbegin++;
            front_storage++;
            length--;
            vector_version++;
        }

        class const_iterator;
        /**********************iterator class*********************************/
        class iterator{
        private:
            T* ptr;
            uint64_t index;
            uint64_t iterator_version;
            uint64_t record_reallocate_times;
            vector<T>* parent;
            uint64_t inside;   //to keep state of iterator whether it was out of bound previously

            friend class const_iterator;

        public:
            typedef T value_type;
            typedef std::random_access_iterator_tag iterator_category;
            typedef int difference_type;
            typedef T*  pointer;
            typedef T& reference;


            iterator(void) {
                ptr = NULL; parent = NULL;
                index = 0;  iterator_version = 0; record_reallocate_times = 0;
                inside = 1;
            }

            iterator(vector<T>* parent ,T* ptr){
                this->ptr = ptr;
                this->index = ptr - parent->dbegin;
                this->iterator_version = parent->vector_version;
                this->record_reallocate_times = parent->reallocate_times;
                this->parent = parent;
                if((this->index >= 0) && (this->index < parent-> size()))
                    this->inside = 1;
                else this->inside = 0;
            }

            /*iterator(iterator& it){
                this->ptr = it.ptr;
                this->index = it.index;
                this->iterator_version = it.iterator_version;
                this->record_reallocate_times = it.record_reallocate_times;
                this->parent = it.parent;
                this->inside = it.inside;
            }*/

            T& operator*(void) {  check_exception();  return *ptr; }
            T* operator->(void) {  check_exception();  return ptr ; }
            T& operator[](uint64_t k) { check_exception(); return *(ptr+k); }
            const T& operator[](uint64_t k) const { check_exception(); return *(ptr+k); }

            iterator& operator++() {
                check_exception();
                ptr++;
                index++;
                update_inside();
                return *this;
            }

            iterator operator++(int) {
                check_exception();
                iterator tmp{*this};
                this->operator++();
                return tmp;
            }

            bool operator==(const iterator& rhs) const{
                check_exception();
                return this->ptr==rhs.ptr;
            }

            bool operator!=(const iterator& rhs) const{
                check_exception();
                return !(*this==rhs);
            }

            iterator& operator--(){
                check_exception();
                ptr--;
                index--;
                update_inside();
                return *this;
            }

            iterator operator--(int){
                check_exception();
                iterator tmp{*this};
                this->operator--();
                return tmp;
            }

            iterator operator+(int64_t k){
                check_exception();
                iterator tmp{*this};
                tmp.ptr +=k;
                tmp.index +=k;
                update_inside();
                return tmp;
            }

            iterator& operator+=(int64_t k){
                check_exception();
                ptr +=k;
                index +=k;
                update_inside();
                return *this;
            }

            iterator operator-(int64_t k){
                check_exception();
                iterator tmp{*this};
                tmp.ptr -=k;
                tmp.index -=k;
                update_inside();
                return tmp;
            }

            iterator& operator-=(int64_t k){
                check_exception();
                ptr -=k;
                index -=k;
                update_inside();
                return *this;
            }

            int64_t operator-(iterator& it){
                return this->ptr - it.ptr;
            }

            bool operator<(iterator& it){
                return (this->index < it.index);
            }

            bool operator>=(iterator& it){
                return (!(this->index < it.index));
            }

            bool operator>(iterator& it){
                return (this->index > it.index);
            }

            bool operator<=(iterator& it){
                return (!(this->index > it.index));
            }

            // others remain to be implemented
            void check_exception() const{
                if(iterator_version != parent->vector_version){
                    if(record_reallocate_times != parent->reallocate_times)
                        throw epl::invalid_iterator{    epl::invalid_iterator::MODERATE };
                    else if( ((index < 0)||(index >= parent->size() )) && inside)
                        throw epl::invalid_iterator{    epl::invalid_iterator::SEVERE };
                    else
                        throw epl::invalid_iterator{    epl::invalid_iterator::MILD  };
                }
            }

            void update_inside(){
                if(  (index >= 0)&&(index < parent->size()   ))
                   inside = 1;
                else
                   inside = 0;
            }

        };
        /*********************iterator class***********************************/


        /*********************const_iterator class******************************/
        class const_iterator{
        private:
            const T* ptr;
            uint64_t index;
            uint64_t iterator_version;
            uint64_t record_reallocate_times;
            const vector<T>* parent;
            uint64_t inside;

        public:
            typedef T value_type;
            typedef std::random_access_iterator_tag iterator_category;
            typedef int difference_type;
            typedef T*  pointer;
            typedef T& reference;

            const_iterator(void){
                ptr = NULL; parent = NULL;
                index = 0;  iterator_version = 0; record_reallocate_times = 0;
                inside = 1;
            }

            const_iterator(const vector<T>* parent ,const T* ptr){
                this->ptr = ptr;
                this->index = ptr - parent->dbegin;
                this->iterator_version = parent->vector_version;
                this->record_reallocate_times = parent->reallocate_times;
                this->parent = parent;
                if((this->index >= 0) && (this->index <  parent-> size()))
                    this->inside = 1;
                else this->inside = 0;
            }

            const_iterator(const iterator& it){
                this->ptr = it.ptr;
                this->index = it.index;
                this->iterator_version = it.iterator_version;
                this->record_reallocate_times = it.record_reallocate_times;
                this->parent = it.parent;
                this->inside = it.inside;
            }  //how to write this function

            //const_iterator& operator=(iterator& it) {}
            const T& operator*(void) const{  check_exception();  return *ptr; }
            const T* operator->(void) const{  check_exception();  return ptr ; }
            //T& operator[](uint64_t k) { check_exception(); return *(ptr+k); }
            const T& operator[](uint64_t k) const { check_exception(); return *(ptr+k); }

            const_iterator& operator++() {
                check_exception();
                ptr++;
                index++;
                update_inside();
                return *this;
            }

            const_iterator operator++(int) {
                check_exception();
                const_iterator tmp{*this};
                this->operator++();
                return tmp;
            }

            bool operator==(const const_iterator& rhs) const{
                check_exception();
                return this->ptr==rhs.ptr;
            }

            bool operator!=(const const_iterator& rhs) const{
                check_exception();
                return !(*this==rhs);
            }

            const_iterator& operator--(){
                check_exception();
                ptr--;
                index--;
                update_inside();
                return *this;
            }

            const_iterator operator--(int){
                check_exception();
                const_iterator tmp{*this};
                this->operator--();
                return tmp;
            }

            const_iterator operator+(int64_t k){
                check_exception();
                const_iterator tmp{*this};
                tmp.ptr +=k;
                tmp.index +=k;
                update_inside();
                return tmp;
            }

            const_iterator& operator+=(int64_t k){
                check_exception();
                ptr +=k;
                index +=k;
                update_inside();
                return *this;
            }

            const_iterator operator-(int64_t k){
                check_exception();
                const_iterator tmp{*this};
                tmp.ptr -=k;
                tmp.index -=k;
                update_inside();
                return tmp;
            }

            const_iterator& operator-=(int64_t k){
                check_exception();
                ptr -=k;
                index -=k;
                update_inside();
                return *this;
            }

            int64_t operator-(const_iterator& it){
                return this->ptr - it.ptr;
            }

            bool operator<(const_iterator& it){
                return (this->index < it.index);
            }

            bool operator>=(const_iterator& it){
                return (!(this->index < it.index));
            }

            bool operator>(const_iterator& it){
                return (this->index > it.index);
            }

            bool operator<=(const_iterator& it){
                return (!(this->index > it.index));
            }

            // others remain to be implemented
            void check_exception() const{
                if(iterator_version != parent->vector_version){
                    if(record_reallocate_times != parent->reallocate_times)
                        throw epl::invalid_iterator{    epl::invalid_iterator::MODERATE };
                    else if( ((index < 0)||(index >=  parent->size() )) && inside)
                        throw epl::invalid_iterator{    epl::invalid_iterator::SEVERE  };
                    else
                        throw epl::invalid_iterator{    epl::invalid_iterator::MILD  };
                }
            }

            void update_inside(){
                if(  (index >= 0)&&(index < parent->size()  ))
                   inside = 1;
                   else
                   inside = 0;
            }
        };
        /*********************const_iterator class******************************/



        /**********************begin and end*********************************/
        const_iterator begin(void) const{
            return const_iterator(this, dbegin);
        }

        const_iterator end(void) const{
            return const_iterator(this, dend);
        }

        iterator begin(void){
            return iterator(this, dbegin);
        }

        iterator end(void){
            return iterator(this, dend);
        }
        /**********************begin and end*********************************/




    private:
        void destroy(){
        	T* tmp = dbegin;
            while(tmp != dend){
                tmp -> ~T();
                tmp++;
            }
            operator delete(sbegin);
        }

        void copy(const vector<T>& that){
            length = that.length;
            storage = that.storage;
            front_storage = that.front_storage;
            sbegin = (T*) (operator new(sizeof(T) * storage));
            send = sbegin + storage;
            dbegin = sbegin + front_storage;
            dend = dbegin  + length;

            for(uint64_t k = 0; k < length ; k++){
                //  error dbegin[k] = that.dbegin[k];
            	new (dbegin + k)  T(that.dbegin[k]);
            }
        }

        /********************move  part b***************************/
        void move(vector<T>&& that){
            length = that.length;
            front_storage = that.front_storage;
            storage = that.storage;

            sbegin = that.sbegin;
            send = that.send;
            dbegin = that.dbegin;
            dend = that.dend;

            that.sbegin = nullptr;
            that.send = nullptr;
            that.dbegin = nullptr;
            that.dend = nullptr;

            vector_version++;
        }


    };

} //namespace epl

#endif
