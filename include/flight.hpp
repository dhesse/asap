#ifndef _FLIGHT_H_
#define _FLIGHT_H_

#include <fstream>
#include <exception>
#include <iostream>
#include <regex>
#include <map>
#include <list>

namespace asap {

  
  // forward declarations
  class Passenger;
  class Seat;

  ////////////////////////////////////////////////////////////
  //
  // Seat type, window, aisle, other. We use this for sorting, so
  // ordering is important. There will be preferences for windwo or
  // aisle, so these should come first. Potentially there are less
  // window seats (since there might be two aisles), and hence those
  // should come first.
  //
  // \author Dirk Hesse <herr.dirk.hesse@gmail.com>
  // \date Sun Oct  6 09:33:26 2013

  enum class SeatType { kWindow, kAisle, kOther };

  ////////////////////////////////////////////////////////////
  //
  // Seat categories, i.e. economy, business, first.
  //
  // \author Dirk Hesse <herr.dirk.hesse@gmail.com>
  // \date Sun Oct  6 10:24:28 2013

  enum class TravelCategory { kFirst, kBusiness, kEconomy };

  namespace detail { // helper classes/functions
    
    inline void get_lower(std::ifstream &in_f, std::string& s){
      in_f >> s;
      std::transform(s.begin(), s.end(), s.begin(), tolower);
    }
    // singleton to map strings -> seat categories
    // and seat_types -> strings
    class CatMap {
    public:
      class NoSuchCategory : public std::exception { };
      bool is_valid_cat(const std::string& str) const {
	return cat_map_.find(str) != cat_map_.end();
      }
      const TravelCategory& cat(const std::string& str) const {
	if (!is_valid_cat(str))
	  throw NoSuchCategory();
	return cat_map_.find(str)->second;
      }
      std::string desc(const SeatType& t) const {
	auto i = seat_map_.find(t);
	if (i != seat_map_.end())
	  return i->second;
	else
	  return "";
      }
      std::string desc(const TravelCategory& t) const {
	auto i = rev_map_.find(t);
	if (i != rev_map_.end())
	  return i->second;
	else
	  return "";
      }
      static const CatMap & instance() {
	static CatMap inst_;
	return inst_;
      }
    private:
      CatMap() {
	cat_map_["economy"] = TravelCategory::kEconomy;
	cat_map_["business"] = TravelCategory::kBusiness;
	cat_map_["first"] = TravelCategory::kFirst;
	rev_map_[TravelCategory::kEconomy] = "economy";
	rev_map_[TravelCategory::kBusiness] = "business" ;
	rev_map_[TravelCategory::kFirst] = "first";
	seat_map_[SeatType::kWindow] = "W";
	seat_map_[SeatType::kAisle] = "A";
	seat_map_[SeatType::kOther] = "";
      }
      std::map<std::string, TravelCategory> cat_map_;
      std::map<TravelCategory, std::string> rev_map_;
      std::map<SeatType, std::string> seat_map_;
    };

    class SeatCreator {
    public:
      explicit SeatCreator(const std::string& label) : 
	label_(label), type_(SeatType::kOther) { }
      std::shared_ptr<Seat> make_seat(const int& row, const int& id, bool is_exit, const double& weight){
	return std::make_shared<Seat>(type_, id, std::to_string(row) + label_, weight, is_exit);
      }
      void set_type(const SeatType& t){
	type_ = t;
      }
    private:
      std::string label_;
      SeatType type_;
    };

    // penalty costs
    const double wrong_seat_penalty = 1;
    const double wrong_sec_penalty = 10;
    const double overbook_penalty = 1000;

    double penalty (const std::shared_ptr<Seat>&, const std::shared_ptr<Passenger> &);

    template <typename Iter>
    void sort_most_restrictive_first (Iter first, Iter last);
    
    template <typename Iter>
    std::pair<Iter, double> find_best_match (Iter first, Iter last, const std::shared_ptr<Passenger>& p);
  
    template <typename Iter1, typename Iter2>
    double match(Iter1 firstp, Iter1 lastp, Iter2 firsts, Iter2 lasts);
    template <typename Iter1, typename Iter2>
    void assign(Iter1 firstp, Iter1 lastp, Iter2 firsts, Iter2 lasts);

  }

  ////////////////////////////////////////////////////////////
  //
  // Seat class.
  //
  // A seat has the following properties:
  //   - Type of seat, e.g. window, aisle, other.
  //   - A seat id, which is used to conveniently determine which seats
  //     are next to each other.
  //   - Descriptive identifier, e.g. "12A" (could be determined from
  //     id).
  //   - cost, which determines how desireable it is to occupy that
  //     seat for reasons of load balancing. Lower numbers are
  //     associated with higher priority.
  //   - Flag if it is at emergency exti.
  //   - A shared pointer to passenger occupying it.
  //
  // \author Dirk Hesse <herr.dirk.hesse@gmail.com>
  // \date Sun Oct  6 09:35:47 2013

  class Seat {
  public:
    Seat(SeatType type, int id, std::string desc = "", double cost = 0,
	 bool is_emergency_exit_seat = false) :
      type_(type), id_(id), desc_(desc), cost_(cost),
      is_emergency_exit_seat_(is_emergency_exit_seat) { }
    std::string get_info() const {
      return desc_ + "(" + detail::CatMap::instance().desc(type_)
	+ (is_emergency_exit_seat_ ? "E)" : ")");
    }
    const SeatType& get_seat_type() const { return type_; }
    bool is_emergency_exit_seat() const {
      return is_emergency_exit_seat_;
    }
    std::shared_ptr<Passenger> get_passenger() const {
      return passenger_;
    }
    int get_id() const { return id_; }
    double get_intrinsic_cost() const { return cost_; }
    void set_passenger(const std::shared_ptr<Passenger>& p) {
      passenger_ = p;
    }
  private:
    SeatType type_;
    int id_;
    std::string desc_;
    double cost_;
    bool is_emergency_exit_seat_;
    std::shared_ptr<Passenger> passenger_;
  };

  class Passenger {
  public:
    Passenger(std::string name, SeatType type, bool minor) :
      name_(name), type_(type), is_minor_(minor) { }
    const SeatType& get_seat_type() const { return type_; }
    bool is_minor() const { return is_minor_; }
    const std::string& get_name() const { return name_; }
  private:
    std::string name_;
    SeatType type_;
    bool is_minor_;
  };
  
  class PassengerGroup {
  public:
    explicit PassengerGroup(TravelCategory cat) : cat_(cat) { }
    void push (const std::string& name, const SeatType& type, bool minor) {
      passengers_.push_back(std::make_shared<Passenger>(name, type, minor));
    }
    typedef std::vector<std::shared_ptr<Passenger> >::iterator iterator;
    typedef std::vector<std::shared_ptr<Passenger> >::const_iterator const_iterator;
    iterator begin() { return passengers_.begin(); }
    iterator end() { return passengers_.end(); }
    const_iterator begin() const { return passengers_.begin(); }
    const_iterator end() const { return passengers_.end(); }
    void sort();
    const TravelCategory& cat() const { return cat_; }
    size_t size() const { return passengers_.size(); }
  private:
    TravelCategory cat_;
    std::vector<std::shared_ptr<Passenger> > passengers_;
  };

  class Flight {
  public:
    enum class AssignResult {kOk, kOverbooked};
    class FileNotFoundError : public std::exception { };
    class InputFileFormatError : public std::exception { };
    explicit Flight(std::string file);
    void show();
    AssignResult checkin(PassengerGroup&);
  private:
    void init(std::ifstream&);
    std::map<TravelCategory, int> rows_;
    std::map<TravelCategory, std::list<int> > emergency_;
    std::map<TravelCategory, int> offset_;
    std::map<TravelCategory, std::vector<std::shared_ptr<Seat> > > seats_by_id_;
    std::map<TravelCategory, std::vector<std::deque<std::shared_ptr<Seat> > > > seats_by_row_;
    std::string flight_number_;
  };
}

#endif // _FLIGHT_H_
