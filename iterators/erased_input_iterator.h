#pragma once
#include <algorithm>
#include <iterator>
#include "../SmallContainers/erasable.hpp"

namespace mpd {
	namespace iterator {
		template<class value_type_, class difference_type_=std::ptrdiff_t, class pointer_=value_type_*, class reference_=value_type_&>
		class erased_input_iterator {
			class erasing {
				virtual ~erasing {};
				virtual bool operator==(const erasing& rhs) const=0;
				virtual reference_ operator*()=0;
				virtual pointer_ operator->()=0;
				virtual void operator++()=0;
				virtual pointer_ uninitialized_copy_data(erasing* end, pointer_ dest)=0; //fundamentally: std::uninitialized_copy(self, end, dest)
				virtual pointer_ copy_data(erasing* end, pointer_ dest)=0; //fundamentally: std::copy(self, end, dest)
				virtual void for_each(erasing* end, std::function<void(reference_)> func)=0; //fundamentally: std::for_each(self, end, func)
			};_n
			template <class it>
			class erased : erasing {
				it base;
				virtual bool operator==(const erasing& rhs_) const {
					const erased& rhs = dynamic_cast<const erased&>(rhs);
					return base==rhs.base;
				}
				virtual reference_ operator*() {return *base;}
				virtual pointer_ operator->() {return base.operator->();}
				virtual void operator++() {++base;}
				virtual pointer_ uninitialized_copy_data(erasing* maybe_end, pointer_ dest) {
					erased& end = dynamic_cast<erased&>(*maybe_end);
					return std::uninitialized_copy(base, end.base, dest);
				}
				virtual pointer_ copy_data(erasing* maybe_end, pointer_ dest) {
					erased& end = dynamic_cast<erased&>(*maybe_end);
					return std::copy(base, end.base, dest);
				}
				virtual void for_each(erasing* maybe_end, std::function<void(reference_)> func) {
					erased& end = dynamic_cast<erased&>(*maybe_end);
					return std::for_each(base, end.base, func);
				}
			};
			erasing<erasing,sizeof(void*)*5, alignof(void*),true> iterptr;
		public:
			using value_type = value_type;
			using difference_type = difference_type_;
			using pointer = pointer_;
			using reference = reference_;
			using iterator_category = std::input_iterator_tag;

			template<class input_iterator, 
				std::enable_if_t<std::is_base_of_v<std::input_iterator_tag, typename std::iterator_traits<input_iterator>::category>>,bool> ==true>
			explicit erased_input_iterator(input_iterator iterator) : iterptr(erased<input_iterator>{iterator}) {}
			erased_input_iterator(const erased_input_iterator& rhs) = default;
			erased_input_iterator(erased_input_iterator&& rhs) = default;
			erased_input_iterator& operator=(const erased_input_iterator& rhs) = default;
			erased_input_iterator& operator=(erased_input_iterator&& rhs) = default;

			bool operator==(const erased_input_iterator& rhs) const {return *iterptr==*rhs;}
			reference_ operator*() {return *iterptr.get();}
			pointer_ operator->() {return iterptr.get().operator->();}
			erased_input_iterator& operator++() {iterptr.get()++; return *this;}
			pointer_ uninitialized_copy(erased_input_iterator end, pointer_ dest) {return iterptr->uninitialized_copy_data(end.iterptr.get(), dest); }
			pointer_ copy(erased_input_iterator end, pointer_ dest) {return iterptr->copy_data(end.iterptr.get(), dest); }
			void for_each(erased_input_iterator end, std::function<void(reference_)> func) {return iterptr->for_each(end.iterptr.get(), func); }
		};
	}
}