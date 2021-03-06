#include "pearl/pearl.h"

#include  <vector>

#include "rapter/typedefs.h"
#include "rapter/util/parse.h"    // console::parse_argument
#include "rapter/io/io.h"         // readPrimitives, readPoints
#include "rapter/util/containers.hpp" // add
#include "rapter/simpleTypes.h"
#include "rapter/util/containers.hpp"
//#include "rapter/primitives/impl/planePrimitive.hpp"


template int
am::Pearl::run(
        std::vector<int>                                        & labels
        , std::vector<rapter::LinePrimitive>      & lines
        , boost::shared_ptr<pcl::PointCloud<pcl::PointXYZRGB> >            const& cloud
        , rapter::PointContainerT const& points
        , std::vector<int>   const* indices
        , am::Pearl::Params                   const& params
        , std::vector<std::vector<int> > *label_history
        , std::vector<std::vector<rapter::LinePrimitive> > *line_history
        , int const nPropose
        );

template int
am::Pearl::run(
        std::vector<int>          & labels
        , std::vector<rapter::PlanePrimitive>      & lines
        , boost::shared_ptr<pcl::PointCloud<pcl::PointXYZRGB> >            const& cloud
        , rapter::PointContainerT                      const& points
        , std::vector<int>                          const* indices
        , am::Pearl::Params                         const& params
        , std::vector<std::vector<int                > > * label_history
        , std::vector<std::vector<rapter::PlanePrimitive> > * line_history
        , int const nPropose
        );

template < typename _PointContainerT
         , class    _InnerPrimitiveContainerT
         , typename _PrimitiveContainerT>
int pearlCli( int argc, char **argv )
{
    typedef typename _InnerPrimitiveContainerT::value_type    PrimitiveT;
    typedef typename _PointContainerT::value_type             PointPrimitiveT;
    typedef          std::map<rapter::GidT, _InnerPrimitiveContainerT> PrimitiveMapT;
    typedef          pcl::PointCloud<pcl::PointNormal>        PclCloudT;

    bool valid_input = true;
    int nPropose = 0;

    std::string cloud_path       = "./cloud.ply",
                input_prims_path = ""; // ./patches.csv";

    am::Pearl::Params params;

    // parse
    {
        if (    (rapter::console::parse_argument( argc, argv, "--cloud", cloud_path) < 0)
             && (!boost::filesystem::exists(cloud_path)) )
        {
            std::cerr << "[" << __func__ << "]: " << "--cloud does not exist: " << cloud_path << std::endl;
            valid_input = false;
        }

        // primitives
        if (    (rapter::console::parse_argument( argc, argv, "-p"     , input_prims_path) < 0)
             && (rapter::console::parse_argument( argc, argv, "--prims", input_prims_path) < 0)
             && (!boost::filesystem::exists(input_prims_path)) )
        {
            if (    (rapter::console::parse_argument( argc, argv, "-N"     , nPropose) < 0) )
            {
                std::cerr << "[" << __func__ << "]: " << "-p or --prims  OR -N (nPropose) is compulsory" << std::endl;
                valid_input = false;
            }
        }

        // scale
        if (    (rapter::console::parse_argument( argc, argv, "--scale", params.scale) < 0)
             && (rapter::console::parse_argument( argc, argv, "-sc"    , params.scale) < 0) )
        {
            std::cerr << "[" << __func__ << "]: " << "--scale is compulsory" << std::endl;
            valid_input = false;
        }

        // weights
        pcl::console::parse_argument( argc, argv, "--unary", params.lambdas(0) );
        pcl::console::parse_argument( argc, argv, "--int-mult", params.int_mult );
        pcl::console::parse_argument( argc, argv, "--cmp"  , params.beta );
        valid_input &= pcl::console::parse_argument( argc, argv, "--pw"  , params.lambdas(2) ) >= 0;

        if (     !valid_input
              || (rapter::console::find_switch(argc,argv,"-h"    ))
              || (rapter::console::find_switch(argc,argv,"--help")) )
        {
            std::cout << "[" << __func__ << "]: " << "Usage: " << argv[0] << "\n"
                      << "\t --cloud " << cloud_path << "\n"
                      << "\t -p,--prims " << input_prims_path << "\n"
                      << "\t -sc,--scale " << params.scale << "\n"
                      << "\t -N " << nPropose << "\n"
                      << "\t --pw " << params.lambdas(2) << "\n"
                      << "\t --cmp " << params.beta << "\n"
                      << "\t --unary " << params.lambdas(0) << "\n"
                      << "\t --int-mult " << params.int_mult << "\n"
                      << "\n\t Example: ../pearl --scale 0.03 --cloud cloud.ply -p patches.csv --pw 1000 --cmp 1000 --int-mult 1000\n"
                      << "\n";

            return EXIT_FAILURE;
        }
    } //...parse

    int err = EXIT_SUCCESS;

    // READ
    _PointContainerT points;
    PclCloudT::Ptr   pcl_cloud( new PclCloudT );
    if ( EXIT_SUCCESS == err )
    {
        err = rapter::io::readPoints<PointPrimitiveT>( points, cloud_path, &pcl_cloud );
        if ( err != EXIT_SUCCESS )  std::cerr << "[" << __func__ << "]: " << "readPoints returned error " << err << std::endl;
    } //...read points

    _PrimitiveContainerT initial_primitives;
    PrimitiveMapT        patches;
    if ( EXIT_SUCCESS == err && !input_prims_path.empty() )
    {
        std::cout << "[" << __func__ << "]: " << "reading primitives from " << input_prims_path << "...";
        rapter::io::readPrimitives<PrimitiveT, _InnerPrimitiveContainerT>( initial_primitives, input_prims_path, &patches );
        std::cout << "reading primitives ok (#: " << initial_primitives.size() << ")\n";
    } //...read primitives

    // WORK
    {
        std::vector<int>          labels;
        _InnerPrimitiveContainerT primitives;
        // add input initialization, if exists
        if ( patches.size() )
        {
            for ( typename rapter::containers::PrimitiveContainer<PrimitiveT>::Iterator it( patches ); it.hasNext(); it.step() )
                primitives.push_back( *it );
        }

        err = am::Pearl::run( /* [out]        labels: */ labels
                            , /* [out]    primitives: */ primitives
                            , /* [in]      pcl_cloud: */ pcl_cloud
                            , /* [in]         points: */ points
                            , /* [in]      p_indices: */ NULL
                            , /* [in]    pearlParams: */ params
                            , /* [out] label_history: */ NULL
                            , /* [out]  prim_history: */ (std::vector<std::vector<PrimitiveT> >*) NULL
                            //, /* [in]        patches: */ &initial_primitives
                            , /* [in] nPropose: */ nPropose
                            );

        PrimitiveMapT out_prims;
        std::cout<<"labels:";for(size_t vi=0;vi!=labels.size();++vi)std::cout<<labels[vi]<<" ";std::cout << "\n";
        int tmpgid = 0;
        for ( auto it = primitives.begin(); it != primitives.end(); ++it, ++tmpgid )
            std::cout << it->toString() << std::endl;

        if ( points.size() != labels.size() )
            std::cerr << "[" << __func__ << "]: " << "labels != points " << points.size() << " " << labels.size() << std::endl;
        for ( size_t pid = 0; pid != labels.size(); ++pid )
        {
            const int gid = labels[pid];
            // assign point
            points[pid].setTag( PointPrimitiveT::TAGS::GID, gid );

            // add primitive
            if ( out_prims.find(gid) == out_prims.end() )
            {
                std::cout << "[" << __func__ << "]: " << "adding primitive[" << gid << "]: " << primitives[gid].toString() << std::endl;
                rapter::containers::add( out_prims, gid, primitives[gid] )
                        .setTag( PrimitiveT::TAGS::GID    , gid )
                        .setTag( PrimitiveT::TAGS::DIR_GID, gid );
            }
        }

        rapter::io::writeAssociations<PointPrimitiveT>( points, "./points_primitives.pearl.csv" );
        rapter::io::savePrimitives<PrimitiveT,typename _InnerPrimitiveContainerT::const_iterator>( out_prims, "./primitives.pearl.csv" );
    } //...work

    std::cout << "../show.py -s " << params.scale << " -a points_primitives.pearl.csv -p primitives.pearl.csv" << std::endl;

    return err;
}


int main(int argc, char *argv[])
{
    std::cout << "hello pearl\n";
    if ( rapter::console::find_switch(argc,argv,"--3D") )
    {
        std::cout << "running planes" << std::endl;
        return pearlCli< rapter::PointContainerT
                       , rapter::_3d::InnerPrimitiveContainerT
                       , rapter::_3d::PrimitiveContainerT
                       >( argc, argv );
        return EXIT_FAILURE;
    }
    else
    {
        return pearlCli< rapter::PointContainerT
                       , rapter::_2d::InnerPrimitiveContainerT
                       , rapter::_2d::PrimitiveContainerT
                       >( argc, argv );
    } // if 3D

    return EXIT_FAILURE;
}
