#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <map>

#define ERROR_TAG "Error: "

// hardcoded, because f*** it
#define TERMINAL_WIDTH 80

template<typename T>
void print_vector(std::ostream &out, const std::vector<T> &v)
{
    for(std::size_t i=0;i<v.size()-1;i++)
    {
        out << v[i] << ", ";
    }
    if(v.size() > 0) out << v[v.size()-1];
}

void print_states_vector(std::ostream &out, const std::vector<std::pair<std::string,bool>> &v)
{
    for(std::size_t i=0;i<v.size()-1;i++)
    {
        out << v[i].first << ((v[i].second)?"F":"") << ", ";
    }
    if(v.size() > 0) out << v[v.size()-1].first << ((v[v.size()-1].second)?"F":"");
}

std::string combine_elements(const std::string &e0, const std::string &e1)
{
    return "(" + e0 + "," + e1 + ")";
}

std::vector<std::string> cross_product(const std::vector<std::string> &v0, const std::vector<std::string> &v1)
{
    std::vector<std::string> vret;
    for(std::size_t i=0;i<v0.size();i++)
    {
        for(std::size_t j=0;j<v1.size();j++)
        {
            vret.push_back(combine_elements(v0[i],v1[j]));
        }
    }
    return vret;
}

std::vector<std::pair<std::string,bool>> states_cross_product(const std::vector<std::pair<std::string,bool>> &v0, const std::vector<std::pair<std::string,bool>> &v1)
{
    std::vector<std::pair<std::string,bool>> vret;
    for(std::size_t i=0;i<v0.size();i++)
    {
        for(std::size_t j=0;j<v1.size();j++)
        {
            vret.push_back(std::make_pair(combine_elements(v0[i].first,v1[j].first), v0[i].second && v1[i].second));
        }
    }
    return vret;
}

std::vector<std::string> split_string(const std::string &str, const char delim)
{
    std::stringstream sstr(str);
    std::vector<std::string> ret;
    std::string token;
    while(std::getline(sstr, token, delim))
    {
        ret.push_back(token);
    }
    return ret;
}

class automata
{
    private:
        // my_states[index] = (name, is_final)
        std::vector<std::pair<std::string,bool>> my_states;
        std::vector<std::string> my_alphabet;
        std::string my_start_state;

        // (my_transition_function[input_state])[input_letter] = output_state
        std::map<std::string, std::map<std::string,std::string>> my_transition_function;

    public:
        automata(){}
        automata(const std::vector<std::pair<std::string,bool>> &states, const std::vector<std::string> &alphabet, const std::string &start_state, std::map<std::string, std::map<std::string,std::string>> &transition_function)
        {
            // check if input is valid
            bool ok = false;
            std::size_t final_states = 0;
            for(std::size_t i=0;i<states.size();i++)
            {
                if(states[i].first == start_state) ok = true;
                if(states[i].second) final_states++;
            }
            if(!ok) throw (std::string)"invalid start state: " + start_state;
            if(0 == final_states) std::cout << "Warning: no final states" << std::endl;

            // TODO: this checks only if the all pairs of input state and input letter produce a valid output state
            // it does NOT check if the transition_function is malformed
            // (eg. if there is an extra output state for a certain input state and input letter pair which doesn't exist)
            for(std::size_t i=0;i<states.size();i++)
            {
                for(std::size_t j=0;j<alphabet.size();j++)
                {
                    std::string output_state = (transition_function[states[i].first])[alphabet[j]];
                    ok = false;
                    for(std::size_t k=0;k<states.size();k++)
                    {
                        if(states[k].first == output_state)
                        {
                            ok = true;
                        }
                    }
                    if(!ok) throw (std::string)"transition function contains invalid output state: " + output_state;
                }
            }

            my_states = states;
            my_alphabet = alphabet;
            my_start_state = start_state;
            my_transition_function = transition_function;
        }
        ~automata(){}

        const std::vector<std::pair<std::string,bool>> &get_states() const
        {
            return my_states;
        }

        const std::vector<std::string> &get_alphabet() const
        {
            return my_alphabet;
        }

        const std::string &get_start_state() const
        {
            return my_start_state;
        }

        const std::map<std::string, std::map<std::string,std::string>> &get_transition_function() const
        {
            return my_transition_function;
        }

        automata unify(const automata &a) const
        {
            // (this union a) == inverseof(inverseof(this) intersection inverseof(a))
            automata inv_myself = this->invert();
            automata inv_a = a.invert();
            automata intersection_of_invs = inv_myself.intersect(inv_a);
            return intersection_of_invs.invert();
            
        }
        automata invert() const
        {
            std::vector<std::pair<std::string,bool>> states = this->get_states();
            std::vector<std::string> alphabet = this->get_alphabet();
            std::string start_state = this->get_start_state();
            std::map<std::string, std::map<std::string,std::string>> transition_function = this->get_transition_function();

            // turn final states into normal states and vice versa
            for(std::size_t i=0;i<states.size();i++)
            {
                states[i].second = !states[i].second;
            }

            automata res(states, alphabet, start_state, transition_function);
            return res;
        }
        automata intersect(const automata &a) const
        {
            std::vector<std::pair<std::string,bool>> states = states_cross_product(this->get_states(),a.get_states());
            std::vector<std::string> alphabet = this->get_alphabet();
            if(alphabet != a.get_alphabet()) throw (std::string)"automata have different alphabets.";
            std::string start_state = combine_elements(this->get_start_state(),a.get_start_state());
            std::map<std::string, std::map<std::string,std::string>> transition_function;

            for(std::size_t i=0;i<alphabet.size();i++)
            {
                for(std::size_t j=0;j<this->get_states().size();j++)
                {
                    for(std::size_t k=0;k<a.get_states().size();k++)
                    {
                        //(transition_function[combine_elements(this->get_states()[j].first,a.get_states()[k].first)])[alphabet[i]] =
                        //    combine_elements((this->get_transition_function()[this->get_states()[j].first])[alphabet[i]],
                        //                     (a.get_transition_function()[a.get_states()[k].first])[alphabet[i]]);
//                        std::cout << (this->get_transition_function()[this->get_states()[j].first])[alphabet[i]]<< std::endl;
//                      FUCKING SHIT DOESN'T WORK AND THE COMPILER GIVES ME A SHITTY MESSAGE GRRRRRRR
                        auto x = this->get_transition_function();
                        auto y = x[this->get_states()[j].first];
                        auto z = y[alphabet[i]];
                        auto X = a.get_transition_function();
                        auto Y = X[a.get_states()[k].first];
                        auto Z = Y[alphabet[i]];

                        // AHH FUCK IT WHY DOES IT WORK THIS WAY?

                        (transition_function[combine_elements(this->get_states()[j].first,a.get_states()[k].first)])[alphabet[i]] =
                            combine_elements(z,Z);
                    }
                }
            }

            automata ret(states, alphabet, start_state, transition_function);
            return ret;
        }
        void print(std::ostream &out)
        {
            out << "alphabet: ";
            print_vector(out,my_alphabet);
            out << std::endl;

            out << "states: ";
            print_states_vector(out,my_states);
            out << std::endl;

            out << "start state: ";
            out << my_start_state;
            out << std::endl;

            out << "transition function: " << std::endl;
            out << "d\t|";
            for(std::size_t i=0;i<my_alphabet.size();i++)
            {
                out << my_alphabet[i] << "\t|";
            }
            out << std::endl;
            for(unsigned char i=0;i<TERMINAL_WIDTH;i++) out << "-";
            out << std::endl;

            for(std::size_t i=0;i<my_states.size();i++)
            {
                out << my_states[i].first << ((my_states[i].second)?"F":"") << "\t|";
                for(std::size_t j=0;j<my_alphabet.size();j++)
                {
                    out << (my_transition_function[my_states[i].first])[my_alphabet[j]];
                    out << "\t|";
                }
                out << std::endl;
            }

            
        }
};

automata get_automata(std::istream &in)
{
    std::string s_alphabet, s_states, s_final_states;

    std::vector<std::string> alphabet;
    std::vector<std::string> states_tmp;
    std::vector<std::pair<std::string,bool>> states;
    std::map<std::string, std::map<std::string,std::string>> transition_function;
    std::string start_state;

    std::string output_state;

    std::cout << "Enter the alphabet (one line, letters seperate by a space)" << std::endl;
    std::getline(in, s_alphabet);

    alphabet = split_string(s_alphabet, ' ');

    std::cout << "Enter the states (one line, states seperated by a space, suffix F for final state(s))" << std::endl;
    std::getline(in, s_states);

    states_tmp = split_string(s_states, ' ');
    for(std::size_t i=0;i<states_tmp.size();i++)
    {
        std::string name = states_tmp[i];
        bool state_final = false;
        if('F' == name[name.length()-1])
        {
            name.resize(name.length()-1);
            state_final = true;
        }
        states.push_back(std::make_pair(name,state_final));
    }

    std::cout << "Enter the transition function" << std::endl;
    for(std::size_t i=0;i<states.size();i++)
    {
        for(std::size_t j=0;j<alphabet.size();j++)
        {
            std::cout << "(input state, input letter) = (" << states[i].first << ", " << alphabet[j] << ") -> ";
            std::getline(in, output_state);
            // TODO: maybe check input here... so the user can reenter
            (transition_function[states[i].first])[alphabet[j]] = output_state;
        }
    }

    std::cout << "Enter the start state" << std::endl;
    std::getline(in, start_state);

    automata a(states, alphabet, start_state, transition_function);
    return a;
}

int automata_union()
{
    automata a, b;
    std::cout << "Enter the first automata:" << std::endl;
    try
    {
        a = get_automata(std::cin);
    }
    catch(std::string &str)
    {
        std::cout << str << std::endl;
        return -1;
    }
    std::cout << "Enter the second automata:" << std::endl;
    try
    {
        b = get_automata(std::cin);
    }
    catch(std::string &str)
    {
        std::cout << str << std::endl;
        return -1;
    }
    std::cout << "The union of the given automata is:" << std::endl;
    try
    {
        automata aunionb = a.unify(b);
        aunionb.print(std::cout);
    }
    catch(std::string &str)
    {
        std::cout << ERROR_TAG << str << std::endl;
        return -2;
    }
    return EXIT_SUCCESS;
}

int automata_inverse()
{
    automata a;
    try
    {
        a = get_automata(std::cin);
    }
    catch(std::string &str)
    {
        std::cout << str << std::endl;
        return -1;
    }
    try
    {
        automata ainverse = a.invert();
        std::cout << "The inverse of the given automata is:" << std::endl;
        ainverse.print(std::cout);
    }
    catch(std::string &str)
    {
        std::cout << ERROR_TAG << str << std::endl;
        return -2;
    }
    return EXIT_SUCCESS;
}

int automata_intersection()
{
    automata a, b;
    std::cout << "Enter the first automata:" << std::endl;
    try
    {
        a = get_automata(std::cin);
    }
    catch(std::string &str)
    {
        std::cout << str << std::endl;
        return -1;
    }
    std::cout << "Enter the second automata:" << std::endl;
    try
    {
        b = get_automata(std::cin);
    }
    catch(std::string &str)
    {
        std::cout << str << std::endl;
        return -1;
    }
    std::cout << "The intersection of the given automata is:" << std::endl;
    try
    {
        automata aintersectionb = a.intersect(b);
        aintersectionb.print(std::cout);
    }
    catch(std::string &str)
    {
        std::cout << ERROR_TAG << str << std::endl;
        return -2;
    }
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
    for(int i=1;i<argc;i++)
    {
        std::string option = argv[i];
        if("-u" == option || "--union" == option)
        {
            return automata_union();
        }
        else if("-i" == option || "--inverse" == option)
        {
            return automata_inverse();
        }
        else if("-I" == option || "--intersection" == option)
        {
            return automata_intersection();
        }
        else break;
    }
    std::cout << "Usage: " << argv[0] << "[option]" << std::endl;
    std::cout << "Available options are:" << std::endl;
    std::cout << "-u | --union\t\t\tFind the union of two automata." << std::endl;
    std::cout << "-i | --inverse\t\t\tFind the inverse of one automata." << std::endl;
    std::cout << "-I | --intersection\t\t\tFind the intersection of two automata." << std::endl;
    std::cout << "Note: If multiple options are specified only the first one is executed." << std::endl;
    return -1;
}

