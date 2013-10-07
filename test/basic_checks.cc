////////////////////////////////////////////////////////////
//
// Some unit tests. Usually I'd use googletest, but it compalins when
// using CLANG...
// So here are some very basi known value tests ...
//
// \author Dirk Hesse <herr.dirk.hesse@gmail.com>
// \date Mon Oct  6 10:25:33 2013

#include <flight.hpp>
#include <vector>
#include <string>
#include <memory>

using namespace asap;

struct TestHelper {
  std::vector<SeatType> seat_types;
  std::vector<bool> em_types;
  std::vector<std::shared_ptr<Passenger> > all_passengers;
  std::vector<std::shared_ptr<Seat> > all_seats;
  TestHelper(){
    seat_types = {SeatType::kWindow, SeatType::kAisle, SeatType::kOther};
    em_types = {true, false}; 
    for (auto type : seat_types)
      for (auto em : em_types){
	std::string name  = detail::CatMap::instance().desc(type)
	  + (em ? "-true" : "-false");
	all_passengers.push_back(std::make_shared<Passenger>(name, type, em));
	all_seats.push_back(std::make_shared<Seat>(type, 0, name, 0, em));
      }
  }
};

// check penalties ...
int check_penalties() {
  int result = 0;
  TestHelper helper;
  const double& s = detail::wrong_seat_penalty;
  const double& e = detail::wrong_sec_penalty;
  std::vector<double> known = 
    {e, 0, e + s, s, e + s, s,
     0, 0, s, s, s, s,
     e + s, s, e, 0, s + e, s,
     s, s, 0, 0, s, s,
     e, 0, e, 0, e, 0,
     0, 0, 0, 0, 0, 0};
  size_t count = 0;
  std::string mismatches;
  for ( auto p : helper.all_passengers ) {
    for ( auto s : helper.all_seats ){
      double tmp = abs(known[count] - detail::penalty(s, p));
      if (tmp > std::numeric_limits<double>::epsilon())
	mismatches += "  " + s->get_desc() + " --> " + p->get_name() + "\n";
      result += tmp;
      ++count;
    }
  }
  if (result) {
    std::cout << "Penalties -- ERROR" << std::endl;
    std::cout << "  WRONG penalties for:" << std::endl << mismatches;
  }
  else
    std::cout << "Penalties -- OK" << std::endl;
  return result;
}

// check if the best macht is found for a few cases
int check_find_best_match() {
  int result = 0;
  std::string err_string;
  std::deque<std::shared_ptr<Seat> > seats;
  TestHelper helper;
  for (auto type : helper.seat_types)
    seats.push_back(std::make_shared<Seat>(type, 0, "", 0, false));
  std::vector<SeatType> seat_types = {SeatType::kWindow, SeatType::kAisle};
  for (auto type : seat_types)
    for (auto em : helper.em_types){
      std::shared_ptr<Passenger> p = std::make_shared<Passenger>("John", type, em);
      auto seat = *(detail::find_best_match(seats.begin(), seats.end(), p).first);
      if (seat->get_seat_type() != type){
	err_string += "Seat type '" + detail::CatMap::instance().desc(type) 
	  + "' matched wrongly!\n";
	++result;
      }
    }
  if (result)
    std::cout << "Find best match -- ERROR" << std::endl << err_string;
  else
    std::cout <<  "Find best match -- OK" << std::endl;
  return result;
}

// assign a whole group
int check_assign() {
  int result = 0;
  std::string err_string;
  std::deque<std::shared_ptr<Seat> > seats;
  TestHelper helper;
  PassengerGroup g(TravelCategory::kEconomy);
  for (auto type : helper.seat_types){
    seats.push_back(std::make_shared<Seat>(type, 0, "", 0, false));
    g.push("", type, false);
  }
  g.sort();
  detail::assign(g.begin(), g.end(), seats.begin(), seats.end());
  for (auto i : seats)
    if (!i->get_passenger()){
      err_string += "  category " 
	+  detail::CatMap::instance().desc(i->get_seat_type()) 
	+ "' not assigned!\n";
      ++result;
    }
    else
      if (i->get_seat_type() != i->get_passenger()->get_seat_type()){
	err_string += "  category " 
	  +  detail::CatMap::instance().desc(i->get_seat_type()) 
	  + "' wrongly assigned!\n";
	++result;
      }
  if (result)
    std::cout << "Assign -- ERROR" << std::endl << err_string;
  else
    std::cout <<  "Assign -- OK" << std::endl;
  return result;
}

int main() {
  int result = check_penalties() + check_find_best_match() + check_assign();
  std::cout << result << " tests failed" << std::endl;
  return result;
}
