////////////////////////////////////////////////////////////
//
// ASAP - Airtravel seat assigment program
//        IMPLEMENTATION
//
// \author Dirk Hesse <herr.dirk.hesse@gmail.com>
// \date Mon Oct  7 13:14:26 2013

#include <flight.hpp>
#include <regex>

namespace asap {
  namespace detail {
    double penalty (const std::shared_ptr<Seat>& s, const std::shared_ptr<Passenger>& p) {
      double result = 0;
      if (p->get_seat_type() != s->get_seat_type() 
	  && p->get_seat_type() !=  SeatType::kOther)
	result += wrong_seat_penalty;
      if (p->is_minor() && s->is_emergency_exit_seat())
	result += wrong_sec_penalty;
      return result;
    }
    template <typename Iter>
    void sort_most_restrictive_first (Iter first, Iter last) {
      std::sort(first, last,
		[](const std::shared_ptr<Passenger>& p, 
		   const std::shared_ptr<Passenger>& q){
		  return std::make_pair(!p->is_minor(), p->get_seat_type())
		    < std::make_pair(!q->is_minor(), q->get_seat_type()); });
    }
    template <typename Iter>
    std::pair<Iter, double> find_best_match (Iter first, Iter last,
					     const std::shared_ptr<Passenger>& p) {
      auto iter = std::min_element(first, last,
				   [&p](const std::shared_ptr<Seat>& s,
					const std::shared_ptr<Seat>& t){
				     return penalty(s, p) < penalty(t, p); });
      return std::make_pair(iter, penalty(*iter, p));
    }

    template <typename Iter1, typename Iter2>
    double match(Iter1 firstp, Iter1 lastp, Iter2 firsts, Iter2 lasts){
      double result = 0;
      int count = 0;
      int min_id = std::numeric_limits<int>::max();
      int max_id = 0;
      while (firstp != lastp && firsts != lasts){
	auto j = find_best_match(firsts, lasts, *firstp);
	const double& score = j.second;
	const std::shared_ptr<Seat>& seat_ptr = *(j.first);
	// update scoring for match
	result += score;
	result += seat_ptr->get_intrinsic_cost();
	// update max/min
	if (seat_ptr->get_id() < min_id) min_id = seat_ptr->get_id();
	if (seat_ptr->get_id() > max_id) max_id = seat_ptr->get_id();
	std::swap(*(j.first), *firsts);
	++firsts;
	++firstp;
	++count;
      }
      // penalty for non-contiguous seating
      result += detail::non_contiguous_penalty * (max_id - min_id - count + 1);
      return result;
    }
    
    std::string CatMap::desc(const SeatType& t) const {
      auto i = seat_map_.find(t);
      if (i != seat_map_.end())
	return i->second;
      else
	return "";
    }

    const TravelCategory& CatMap::cat(const std::string& str) const {
      if (!is_valid_cat(str))
	throw NoSuchCategory();
      return cat_map_.find(str)->second;
    }


    const SeatType& CatMap::type(const std::string& str) const {
      if (!is_valid_type(str)){
	std::cerr << "Seat type not recognized: " << std::endl;
	throw NoSuchCategory();
      }
      return type_map_.find(str)->second;
    }
    
    std::string CatMap::desc(const TravelCategory& t) const {
      auto i = rev_map_.find(t);
      if (i != rev_map_.end())
	return i->second;
      else
	return "";
    }

    CatMap::CatMap() {
      cat_map_["economy"] = TravelCategory::kEconomy;
      cat_map_["business"] = TravelCategory::kBusiness;
      cat_map_["first"] = TravelCategory::kFirst;
      type_map_["window"] = SeatType::kWindow;
      type_map_["aisle"] = SeatType::kAisle;
      type_map_["none"] = SeatType::kOther;
      rev_map_[TravelCategory::kEconomy] = "economy";
      rev_map_[TravelCategory::kBusiness] = "business" ;
      rev_map_[TravelCategory::kFirst] = "first";
      seat_map_[SeatType::kWindow] = "W";
      seat_map_[SeatType::kAisle] = "A";
      seat_map_[SeatType::kOther] = "";
    }
    
    // this works optimal only if [firstp, lastp) is
    // sorted using sort_most_restrictive_first
    template <typename Iter1, typename Iter2>
    void assign(Iter1 firstp, Iter1 lastp, Iter2 firsts, Iter2 lasts){
#ifdef DEBUG
      double cost = 0;
      int count = 0;
      int min_id = std::numeric_limits<int>::max();
      int max_id = 0;
#endif
      while (firstp != lastp && firsts != lasts){
	auto j = find_best_match(firsts, lasts, *firstp);
	const double& score = j.second;
	const std::shared_ptr<Seat>& seat_ptr = *(j.first);
#ifdef DEBUG
	cost += score;
	cost += seat_ptr->get_intrinsic_cost();
	++count;
	std::cout << "(" << (*firstp)->get_name() << ","
		  << detail::CatMap::instance().desc((*firstp)->get_seat_type()) 
		  << "," << (*firstp)->is_minor() << ") -> ";
	std::cout << seat_ptr->get_info() << " penalty: " << j.second << std::endl;
	if (seat_ptr->get_id() < min_id) min_id = seat_ptr->get_id();
	if (seat_ptr->get_id() > max_id) max_id = seat_ptr->get_id();
#endif
	seat_ptr->set_passenger(*firstp);
	std::swap(*(j.first), *firsts);
	++firsts;
	++firstp;
      }
#ifdef DEBUG
      // penalty for non-contiguous seating
      cost += max_id - min_id - count + 1;
      std::cout << "max = " << max_id << ", min = " << min_id << ", count = " << count << std::endl;
      std::cout << "TOTAL PENALTY: " << cost << std::endl;
#endif
    }
  } // namespace detail
  
  PassengerGroup::PassengerGroup(const std::string& file){
    std::ifstream inf(file);
    std::string tmp;
    std::string name;
    SeatType st;
    detail::get_lower(inf, tmp);
    cat_ = detail::CatMap::instance().cat(tmp);
    while (inf.good()) {
      inf >> name;
      detail::get_lower(inf, tmp);
      st = detail::CatMap::instance().type(tmp);
      detail::get_lower(inf, tmp);
      bool is_minor = tmp == "minor";
      push(name, st, is_minor);
    }
  }

  void Flight::init (std::ifstream& file) {
    TravelCategory cat;
    static std::regex re_ws("\\W+"); // sarch for whitepace -- separating seats
    static std::regex re_comma(","); // sarch for commas -- separating seat
    // groups
    int total_rows = 1; // start seat numbering with one
    std::string tmp;
    std::map<TravelCategory, std::list<detail::SeatCreator> > seat_creators;
    while (file.good()) {
      detail::get_lower(file, tmp);
      if (detail::CatMap::instance().is_valid_cat(tmp))
	cat = detail::CatMap::instance().cat(tmp);
      else if (tmp == "rows"){
	file >> rows_[cat];
	offset_[cat] = total_rows;
	total_rows += rows_[cat];
      }
      else if (tmp == "emergency") {
	int tmpi;
	file >> tmpi;
	emergency_[cat].push_back(tmpi);
      }
      else if (tmp == "seats"){
	std::getline(file, tmp);
	for(std::sregex_token_iterator i(tmp.begin(), tmp.end(), re_comma, -1), 
	      end; i != end; ++i){
	  std::list<detail::SeatCreator> group;
	  for(std::sregex_token_iterator j(i->first, i->second, re_ws, -1); j != end; ++j)
	    if (j->first != j->second) // avoid extra white space
	      group.push_back(detail::SeatCreator(*j));
	  // mark first and last place in group as aisle
	  group.front().set_type(SeatType::kAisle); 
	  group.back().set_type(SeatType::kAisle);
	  seat_creators[cat].splice(seat_creators[cat].end(), group);
	}
	// mark first and last place in row as window
	seat_creators[cat].front().set_type(SeatType::kWindow);
	seat_creators[cat].back().set_type(SeatType::kWindow);
      }
      else if (tmp == "center") file >> center_at_[cat];
      else continue; // ignore unknown commands
      
    }
    size_t id = 0;
    for (auto const& o : offset_){
      const TravelCategory& cat = o.first;
      seats_by_row_[cat].resize(rows_[cat]);
      for (int i = 0; i < rows_[cat]; ++i){
	int row_number = i + offset_[cat];
	// introduce a weight penalty depending on how far off-center
	// a seat is 
	double weight = detail::weight_penalty*abs(row_number - center_at_[cat]);
	// mark exit seats
	bool is_exit = std::find(emergency_[cat].begin(), 
				 emergency_[cat].end(), 
				   row_number) != emergency_[cat].end();
	// alternate with filling rows left-to-right and
	// right-to-left to have some basic load balancing
	if (i % 2)
	  for (auto creator = seat_creators[cat].begin();
	       creator != seat_creators[cat].end(); ++creator){
	    empty_seats_by_id_[cat].push_back(creator->make_seat(i + o.second, 
								 id++, is_exit, weight));
	    seats_by_row_[cat][i].push_back(empty_seats_by_id_[cat].back());
	  }
	else
	  for (auto creator = seat_creators[cat].rbegin();
		 creator != seat_creators[cat].rend(); ++creator){
	    empty_seats_by_id_[cat].push_back(creator->make_seat(i + o.second,
								 id++, is_exit, weight));
	    seats_by_row_[cat][i].push_front(empty_seats_by_id_[cat].back());
	  }
      }
    }
  } // Flight::init

  void Flight::show() {
    std::cout << "FLIGHT " << flight_number_ << std::endl;
    for (auto rows : seats_by_row_){
      // print category
      std::cout << "---------  " 
		<< detail::CatMap::instance().desc(rows.first) 
		<< "  ---------"
		<< std::endl;
      for (int row = 0; row < rows.second.size(); ++row){
	std::cout << row + offset_[rows.first] << ": ";
	for (auto seat : rows.second[row]){
	  std::cout << seat->get_info() << "::";
	  if (seat->get_passenger())
	    std::cout << seat->get_passenger()->get_name();
	  else
	    std::cout << "NONE";
	  std::cout << ", ";
	}
	std::cout << std::endl;
      }
    }
  } // Flight::show
  
  Flight::Flight(std::string file) {
    std::ifstream input_file(file);
    if (!input_file.is_open())
      throw FileNotFoundError();
    std::string tmp;
    detail::get_lower(input_file, tmp);
    if (tmp != "flight")
      throw InputFileFormatError();
    input_file >> flight_number_;
    init(input_file);
  }

  void PassengerGroup::sort() {
    detail::sort_most_restrictive_first(begin(), end());
  };
  Flight::AssignResult Flight::checkin(PassengerGroup& g){
    AssignResult result = AssignResult::kOk;
    const TravelCategory& cat = g.cat(); // shorthand
    g.sort();
    // another shorthand
    std::vector<std::shared_ptr<Seat> >& empty_seats = empty_seats_by_id_[cat];
    // report overbooking
    if (empty_seats.size() < g.size())
      result = AssignResult::kOverbooked;
    // find best set of empty seats
    auto first = empty_seats.begin();
    auto last = empty_seats.begin() + std::min(g.size(), empty_seats.size());
    std::deque<std::shared_ptr<Seat> > best_so_far(first, last); // reminer: assings [first, last)
    std::deque<std::shared_ptr<Seat> > current = best_so_far;
    double best_score = detail::match(g.begin(), g.end(), current.begin(), current.end());
    while (last != empty_seats.end()){
      current.assign(++first, ++last);
      double new_score = detail::match(g.begin(), g.end(), current.begin(), current.end());
      // additional penalty for sitting directly next 
      // to a passenger from another group
      // TODO: By looking at occupied seats one could find the min
      //       distance to an occupied seat and do something more
      //       sophisticated here
      if ((last+1) != empty_seats.end() && last != empty_seats.end())
	if ((*(last+1))->get_id() - 1 != (*last)->get_id())
	  new_score += detail::neighbor_seat_occupied_penalty;
      if (first != empty_seats.begin())
	if ((*(first-1))->get_id() + 1 != (*first)->get_id())
	  new_score += detail::neighbor_seat_occupied_penalty;
      if (new_score < best_score){
	best_so_far.assign(first, last);
	best_score = new_score;
      }
    }
    detail::assign(g.begin(), g.end(), best_so_far.begin(), best_so_far.end());
    // safely remove occupied seats from vector
    empty_seats.erase(std::remove_if(empty_seats.begin(),
				     empty_seats.end(),
				     [](const std::shared_ptr<Seat>& s){
				       return s->get_passenger(); }),
		      empty_seats.end());
#ifdef DEBUG
    std::cout << "Assigned with score " << best_score << std::endl;
#endif
    return result;
  }
  Flight::AssignResult Flight::checkin(TravelCategory cat, const std::string& name,
				       SeatType seat, bool is_minor) {
    PassengerGroup g(cat);
    g.push(name, seat, is_minor);
    return checkin(g);
  }

  Flight::AssignResult Flight::checkin(TravelCategory cat, const std::string& name,
				       bool is_minor, const std::string& seat_no){
    // shorthand
    const std::vector<std::shared_ptr<Seat> >& seats = empty_seats_by_id_[cat];
    if (seats.empty())
      return AssignResult::kOverbooked;
    auto seat = std::find_if(seats.begin(), seats.end(),
			[&seat_no](const std::shared_ptr<Seat>& s){
			  return s->get_desc() == seat_no;});
    if (seat != seats.end())
      (*seat)->set_passenger(std::make_shared<Passenger>
			     (name, SeatType::kOther, is_minor));
    else {
      std::cerr << "seat not found\n";
      return AssignResult::kSeatUnavailable;
    }
    return AssignResult::kOk;
  }
}// namespace asap
