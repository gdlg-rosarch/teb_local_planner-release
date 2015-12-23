/*********************************************************************
 *
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2015,
 *  TU Dortmund - Institute of Control Theory and Systems Engineering.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the institute nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 * Author: Christoph Rösmann
 *********************************************************************/

#include <teb_local_planner/optimal_planner.h>


namespace teb_local_planner
{

// ============== Implementation ===================

TebOptimalPlanner::TebOptimalPlanner() : cfg_(NULL), obstacles_(NULL), initialized_(false), optimized_(false)
{    
  cost_.setConstant(HUGE_VAL);
}
  
TebOptimalPlanner::TebOptimalPlanner(const TebConfig& cfg, std::vector<ObstaclePtr>* obstacles, TebVisualizationPtr visual)
{    
  initialize(cfg, obstacles, visual);
}

TebOptimalPlanner::~TebOptimalPlanner()
{
  clearGraph();
  // free dynamically allocated memory
  //if (optimizer_) 
  //  g2o::Factory::destroy();
  //g2o::OptimizationAlgorithmFactory::destroy();
  //g2o::HyperGraphActionLibrary::destroy();
}

void TebOptimalPlanner::initialize(const TebConfig& cfg, std::vector<ObstaclePtr>* obstacles, TebVisualizationPtr visual)
{    
  // init optimizer (set solver and block ordering settings)
  optimizer_ = initOptimizer();
  
  cfg_ = &cfg;
  obstacles_ = obstacles;
  cost_.setConstant(HUGE_VAL);
  setVisualization(visual);
  
  vel_start_.first = true;
  vel_start_.second.setZero();

  vel_goal_.first = true;
  vel_goal_.second.setZero();
  initialized_ = true;
}


void TebOptimalPlanner::setVisualization(TebVisualizationPtr visualization)
{
  visualization_ = visualization;
}

void TebOptimalPlanner::visualize()
{
  if (visualization_)
  {
    visualization_->publishLocalPlanAndPoses(teb_);
  }
  else ROS_DEBUG("Ignoring TebOptimalPlanner::visualize() call, since no visualization class was instantiated before.");
}


/*
 * registers custom vertices and edges in g2o framework
 */
void TebOptimalPlanner::registerG2OTypes()
{
  g2o::Factory* factory = g2o::Factory::instance();
  factory->registerType("VERTEX_POSE", new g2o::HyperGraphElementCreator<VertexPose>);
  factory->registerType("VERTEX_TIMEDIFF", new g2o::HyperGraphElementCreator<VertexTimeDiff>);

  factory->registerType("EDGE_TIME_OPTIMAL", new g2o::HyperGraphElementCreator<EdgeTimeOptimal>);
  factory->registerType("EDGE_VELOCITY", new g2o::HyperGraphElementCreator<EdgeVelocity>);
  factory->registerType("EDGE_ACCELERATION", new g2o::HyperGraphElementCreator<EdgeAcceleration>);
  factory->registerType("EDGE_ACCELERATION_START", new g2o::HyperGraphElementCreator<EdgeAccelerationStart>);
  factory->registerType("EDGE_ACCELERATION_GOAL", new g2o::HyperGraphElementCreator<EdgeAccelerationGoal>);
  factory->registerType("EDGE_KINEMATICS", new g2o::HyperGraphElementCreator<EdgeKinematics>);
  factory->registerType("EDGE_POINT_OBSTACLE", new g2o::HyperGraphElementCreator<EdgePointObstacle>);
  factory->registerType("EDGE_LINE_OBSTACLE", new g2o::HyperGraphElementCreator<EdgeLineObstacle>);
  factory->registerType("EDGE_POLYGON_OBSTACLE", new g2o::HyperGraphElementCreator<EdgePolygonObstacle>);
  factory->registerType("EDGE_DYNAMIC_OBSTACLE", new g2o::HyperGraphElementCreator<EdgeDynamicObstacle>);
  return;
}

/*
 * initialize g2o optimizer. Set solver settings here.
 * Return: pointer to new SparseOptimizer Object.
 */
boost::shared_ptr<g2o::SparseOptimizer> TebOptimalPlanner::initOptimizer()
{
  // Call register_g2o_types once, even for multiple TebOptimalPlanner instances (thread-safe)
  static boost::once_flag flag = BOOST_ONCE_INIT;
  boost::call_once(&registerG2OTypes, flag);  

  // allocating the optimizer
  boost::shared_ptr<g2o::SparseOptimizer> optimizer = boost::make_shared<g2o::SparseOptimizer>();
  TEBLinearSolver* linearSolver = new TEBLinearSolver(); // see typedef in optimization.h
  linearSolver->setBlockOrdering(true);
  TEBBlockSolver* blockSolver = new TEBBlockSolver(linearSolver);
  g2o::OptimizationAlgorithmLevenberg* solver = new g2o::OptimizationAlgorithmLevenberg(blockSolver);

  optimizer->setAlgorithm(solver);
  
  optimizer->initMultiThreading(); // required for >Eigen 3.1
  
  return optimizer;
}


bool TebOptimalPlanner::optimizeTEB(unsigned int iterations_innerloop, unsigned int iterations_outerloop, bool compute_cost_afterwards)
{
  if (cfg_->optim.optimization_activate==false) 
    return false;
  bool success = false;
  optimized_ = false;
  for(unsigned int i=0; i<iterations_outerloop; ++i)
  {
    if (cfg_->trajectory.teb_autosize)
      teb_.autoResize(cfg_->trajectory.dt_ref, cfg_->trajectory.dt_hysteresis, cfg_->trajectory.min_samples);

    success = buildGraph();
    if (!success) 
    {
        clearGraph();
        return false;
    }
    success = optimizeGraph(iterations_innerloop, false);
    if (!success) 
    {
        clearGraph();
        return false;
    }
    optimized_ = true;
    
    if (compute_cost_afterwards && i==iterations_outerloop-1) // compute cost vec only in the last iteration
      computeCurrentCost(cfg_->optim.alternative_time_cost);
      
    clearGraph();
  }

  return true;
}

void TebOptimalPlanner::setVelocityStart(const Eigen::Ref<const Eigen::Vector2d>& vel_start)
{
  vel_start_.first = true;
  vel_start_.second = vel_start;
}

void TebOptimalPlanner::setVelocityStart(const geometry_msgs::Twist& vel_start)
{
  vel_start_.first = true;
  vel_start_.second.coeffRef(0) = vel_start.linear.x;
  vel_start_.second.coeffRef(1) = vel_start.angular.z;
}

void TebOptimalPlanner::setVelocityGoal(const Eigen::Ref<const Eigen::Vector2d>& vel_goal)
{
  vel_goal_.first = true;
  vel_goal_.second = vel_goal;
}

bool TebOptimalPlanner::plan(const std::vector<geometry_msgs::PoseStamped>& initial_plan, const geometry_msgs::Twist* start_vel, bool free_goal_vel)
{    
  ROS_ASSERT_MSG(initialized_, "Call initialize() first.");
  if (!teb_.isInit())
  {
    // init trajectory
    teb_.initTEBtoGoal(initial_plan, cfg_->trajectory.dt_ref, true, cfg_->trajectory.min_samples);
  } 
  else
  {
    // update TEB
    PoseSE2 start_(initial_plan.front().pose.position.x, initial_plan.front().pose.position.y, tf::getYaw(initial_plan.front().pose.orientation));
    PoseSE2 goal_(initial_plan.back().pose.position.x, initial_plan.back().pose.position.y, tf::getYaw(initial_plan.back().pose.orientation));
    teb_.updateAndPruneTEB(start_, goal_, cfg_->trajectory.force_reinit_new_goal_dist, cfg_->trajectory.min_samples);
  }
  if (start_vel)
    setVelocityStart(*start_vel);
  if (free_goal_vel)
    setVelocityGoalFree();
  else
    vel_goal_.first = true; // we just reactivate and use the previously set velocity (should be zero if nothing was modified)
  
  // now optimize
  return optimizeTEB(cfg_->optim.no_inner_iterations, cfg_->optim.no_outer_iterations);
}


bool TebOptimalPlanner::plan(const tf::Pose& start, const tf::Pose& goal, const geometry_msgs::Twist* start_vel, bool free_goal_vel)
{
  PoseSE2 start_(start.getOrigin().x(), start.getOrigin().y(), tf::getYaw(start.getRotation()));
  PoseSE2 goal_(goal.getOrigin().x(), goal.getOrigin().y(), tf::getYaw(goal.getRotation()));
  Eigen::Vector2d vel = start_vel ? Eigen::Vector2d(start_vel->linear.x, start_vel->angular.z) : Eigen::Vector2d::Zero();
  return plan(start_, goal_, vel);
}

bool TebOptimalPlanner::plan(const PoseSE2& start, const PoseSE2& goal, const Eigen::Vector2d& start_vel, bool free_goal_vel)
{	
  ROS_ASSERT_MSG(initialized_, "Call initialize() first.");
  if (!teb_.isInit())
  {
    // init trajectory
    teb_.initTEBtoGoal(start, goal, 0, 1, cfg_->trajectory.min_samples); // 0 intermediate samples, but dt=1 -> autoResize will add more samples before calling first optimization
  }
  else
  {
    // update TEB
    teb_.updateAndPruneTEB(start, goal, cfg_->trajectory.force_reinit_new_goal_dist, cfg_->trajectory.min_samples);
  }
  setVelocityStart(start_vel);
  if (free_goal_vel)
    setVelocityGoalFree();
  else
    vel_goal_.first = true; // we just reactivate and use the previously set velocity (should be zero if nothing was modified)
      
  // now optimize
  return optimizeTEB(cfg_->optim.no_inner_iterations, cfg_->optim.no_outer_iterations);
}


bool TebOptimalPlanner::buildGraph()
{
  if (!optimizer_->edges().empty() || !optimizer_->vertices().empty())
  {
    ROS_WARN("Cannot build graph, because it is not empty. Call graphClear()!");
    return false;
  }
  
  // add TEB vertices
  AddTEBVertices();
  
  // add Edges (local cost functions)
  AddEdgesObstacles();
  AddEdgesDynamicObstacles();
  
  AddEdgesVelocity();
  
  AddEdgesAcceleration();

  AddEdgesTimeOptimal();	
  
  AddEdgesKinematics();

  
  return true;  
}

bool TebOptimalPlanner::optimizeGraph(int no_iterations,bool clear_after)
{
  if (cfg_->robot.max_vel_x<0.01)
  {
    ROS_WARN("optimizeGraph(): Robot Max Velocity is smaller than 0.01m/s. Optimizing aborted...");
    if (clear_after) clearGraph();
    return false;	
  }
  
  if (!teb_.isInit() || teb_.sizePoses()<cfg_->trajectory.min_samples)
  {
    ROS_WARN("optimizeGraph(): TEB is empty or has too less elements. Skipping optimization.");
    if (clear_after) clearGraph();
    return false;	
  }
  
  optimizer_->setVerbose(cfg_->optim.optimization_verbose);
  optimizer_->initializeOptimization();

  int iter = optimizer_->optimize(no_iterations);
  
  if(!iter)
  {
	ROS_ERROR("optimizeGraph(): Optimization failed! iter=%i", iter);
	return false;
  }

  if (clear_after) clearGraph();	
    
  return true;
}

void TebOptimalPlanner::clearGraph()
{
  // clear optimizer states
  //optimizer.edges().clear(); // optimizer.clear deletes edges!!! Therefore do not run optimizer.edges().clear()
  optimizer_->vertices().clear();  // neccessary, because optimizer->clear deletes pointer-targets (therefore it deletes TEB states!)
  optimizer_->clear();	
}



void TebOptimalPlanner::AddTEBVertices()
{
  // add vertices to graph
  ROS_DEBUG_COND(cfg_->optim.optimization_verbose, "Adding TEB vertices ...");
  unsigned int id_counter = 0; // used for vertices ids
  for (unsigned int i=0; i<teb_.sizePoses(); ++i)
  {
    teb_.PoseVertex(i)->setId(id_counter++);
    optimizer_->addVertex(teb_.PoseVertex(i));
    if (teb_.sizeTimeDiffs()!=0 && i<teb_.sizeTimeDiffs())
    {
      teb_.TimeDiffVertex(i)->setId(id_counter++);
      optimizer_->addVertex(teb_.TimeDiffVertex(i));
    }
  } 
}



void TebOptimalPlanner::AddEdgesObstacles()
{
  if (cfg_->optim.weight_point_obstacle==0 || obstacles_==NULL )
    return; // if weight equals zero skip adding edges!

  ROS_DEBUG_COND(cfg_->optim.optimization_verbose, "Adding obstacle edges ...");
  for (ObstContainer::const_iterator obst = obstacles_->begin(); obst != obstacles_->end(); ++obst)
  {
    if ((*obst)->isDynamic()) // we handle dynamic obstacles differently below
      continue; 
    
    const PointObstacle* point_obst = dynamic_cast<const PointObstacle*>(obst->get());
    const LineObstacle* line_obst = dynamic_cast<const LineObstacle*>(obst->get());
    const PolygonObstacle* poly_obst = dynamic_cast<const PolygonObstacle*>(obst->get());
    unsigned int index;
    if(point_obst || poly_obst)
		{
      index = teb_.findClosestTrajectoryPose((*obst)->getCentroid()); // TODO: not valid for generic polygons
    }
    else if(line_obst)
    { 
      index = teb_.findClosestTrajectoryPose(line_obst->start(), line_obst->end());
    }
    else
    {
      ROS_ERROR("Unknown obstacle type found, skipping...");
      continue;
    }
    
    
    // check if obstacle is outside index-range between start and goal
    if ( (index <= 1) || (index > teb_.sizePoses()-2) ) // start and goal are fixed and findNearestBandpoint finds first or last conf if intersection point is outside the range
	    continue;

    if (point_obst)
    {
      Eigen::Matrix<double,1,1> information;
      information.fill(cfg_->optim.weight_point_obstacle);
      EdgePointObstacle* dist_bandpt_obst = new EdgePointObstacle;
      dist_bandpt_obst->setVertex(0,teb_.PoseVertex(index));
      dist_bandpt_obst->setInformation(information);
      dist_bandpt_obst->setObstaclePosition(point_obst->getCentroid());   
      dist_bandpt_obst->setTebConfig(*cfg_);
      optimizer_->addEdge(dist_bandpt_obst);

      for (unsigned int neighbourIdx=0; neighbourIdx < floor(cfg_->obstacles.obstacle_poses_affected/2); neighbourIdx++)
      {
        if (index+neighbourIdx < teb_.sizePoses())
        {
          EdgePointObstacle* dist_bandpt_obst_n_r = new EdgePointObstacle;
          dist_bandpt_obst_n_r->setVertex(0,teb_.PoseVertex(index+neighbourIdx));
          dist_bandpt_obst_n_r->setInformation(information);
          dist_bandpt_obst_n_r->setObstaclePosition(point_obst->getCentroid());
          dist_bandpt_obst_n_r->setTebConfig(*cfg_);
          optimizer_->addEdge(dist_bandpt_obst_n_r);
        }
        if ( (int) index - (int) neighbourIdx >= 0) // needs to be casted to int to allow negative values
        {
          EdgePointObstacle* dist_bandpt_obst_n_l = new EdgePointObstacle;
          dist_bandpt_obst_n_l->setVertex(0,teb_.PoseVertex(index-neighbourIdx));
          dist_bandpt_obst_n_l->setInformation(information);
          dist_bandpt_obst_n_l->setObstaclePosition(point_obst->getCentroid());
          dist_bandpt_obst_n_l->setTebConfig(*cfg_);
          optimizer_->addEdge(dist_bandpt_obst_n_l);
        }
      }
      continue;
    }

    if (line_obst)
    {
      Eigen::Matrix<double,1,1> information;
      information.fill(cfg_->optim.weight_line_obstacle);
      EdgeLineObstacle* dist_bandpt_obst = new EdgeLineObstacle;
      dist_bandpt_obst->setVertex(0,teb_.PoseVertex(index));
      dist_bandpt_obst->setInformation(information);
      dist_bandpt_obst->setMeasurement(line_obst);
      dist_bandpt_obst->setTebConfig(*cfg_);
      optimizer_->addEdge(dist_bandpt_obst);

      for (unsigned int neighbourIdx=0; neighbourIdx < floor(cfg_->obstacles.line_obstacle_poses_affected/2); neighbourIdx++)
      {
        if (index+neighbourIdx < teb_.sizePoses())
        {
          EdgeLineObstacle* dist_bandpt_obst_n_r = new EdgeLineObstacle;
          dist_bandpt_obst_n_r->setVertex(0,teb_.PoseVertex(index+neighbourIdx));
          dist_bandpt_obst_n_r->setInformation(information);
          dist_bandpt_obst_n_r->setMeasurement(line_obst);
          dist_bandpt_obst_n_r->setTebConfig(*cfg_);
          optimizer_->addEdge(dist_bandpt_obst_n_r);
        }
        if ( (int) index - (int) neighbourIdx >= 0) // needs to be casted to int to allow negative values
        {
          EdgeLineObstacle* dist_bandpt_obst_n_l = new EdgeLineObstacle;
          dist_bandpt_obst_n_l->setVertex(0,teb_.PoseVertex(index-neighbourIdx));
          dist_bandpt_obst_n_l->setInformation(information);
          dist_bandpt_obst_n_l->setMeasurement(line_obst);
          dist_bandpt_obst_n_l->setTebConfig(*cfg_);
          optimizer_->addEdge(dist_bandpt_obst_n_l);
        }
      }
      continue;
    }

    if (poly_obst)
    {
      Eigen::Matrix<double,1,1> information;
      information.fill(cfg_->optim.weight_poly_obstacle);
      EdgePolygonObstacle* dist_bandpt_obst = new EdgePolygonObstacle;
      dist_bandpt_obst->setVertex(0,teb_.PoseVertex(index));
      dist_bandpt_obst->setInformation(information);
      dist_bandpt_obst->setMeasurement(poly_obst);    
      dist_bandpt_obst->setTebConfig(*cfg_);
      optimizer_->addEdge(dist_bandpt_obst);

      for (unsigned int neighbourIdx=0; neighbourIdx < floor(cfg_->obstacles.polygon_obstacle_poses_affected/2); neighbourIdx++)
      {
        if (index+neighbourIdx < teb_.sizePoses())
        {
          EdgePolygonObstacle* dist_bandpt_obst_n_r = new EdgePolygonObstacle;
          dist_bandpt_obst_n_r->setVertex(0,teb_.PoseVertex(index+neighbourIdx));
          dist_bandpt_obst_n_r->setInformation(information);
          dist_bandpt_obst_n_r->setMeasurement(poly_obst);
          dist_bandpt_obst_n_r->setTebConfig(*cfg_);
          optimizer_->addEdge(dist_bandpt_obst_n_r);
        }
        if ( (int) index - (int) neighbourIdx >= 0) // needs to be casted to int to allow negative values
        {
          EdgePolygonObstacle* dist_bandpt_obst_n_l = new EdgePolygonObstacle;
          dist_bandpt_obst_n_l->setVertex(0,teb_.PoseVertex(index-neighbourIdx));
          dist_bandpt_obst_n_l->setInformation(information);
          dist_bandpt_obst_n_l->setMeasurement(poly_obst);
          dist_bandpt_obst_n_l->setTebConfig(*cfg_);
          optimizer_->addEdge(dist_bandpt_obst_n_l);
        }
      }
	  
      continue;
    }
	  
  }
}

void TebOptimalPlanner::AddEdgesDynamicObstacles()
{
  if (cfg_->optim.weight_point_obstacle==0 || obstacles_==NULL )
    return; // if weight equals zero skip adding edges!
  
  Eigen::Matrix<double,1,1> information;
  information.fill(cfg_->optim.weight_dynamic_obstacle);
  ROS_DEBUG_COND(cfg_->optim.optimization_verbose, "Adding dynamic obstacle edges ...");
  
  for (ObstContainer::const_iterator obst = obstacles_->begin(); obst != obstacles_->end(); ++obst)
  {
    if (!(*obst)->isDynamic())
      continue;

    for (std::size_t i=1; i < teb_.sizePoses() - 1; ++i)
    {
      EdgeDynamicObstacle* dynobst_edge = new EdgeDynamicObstacle(i);
      dynobst_edge->setVertex(0,teb_.PoseVertex(i));
      //dynobst_edge->setVertex(1,teb.PointVertex(i+1));
      dynobst_edge->setVertex(1,teb_.TimeDiffVertex(i));
      dynobst_edge->setInformation(information);
      dynobst_edge->setMeasurement(obst->get());
      dynobst_edge->setTebConfig(*cfg_);
      optimizer_->addEdge(dynobst_edge);
    }
  }
}

void TebOptimalPlanner::AddEdgesVelocity()
{
  if (cfg_->optim.weight_max_vel_x==0 && cfg_->optim.weight_max_vel_theta==0)
    return; // if weight equals zero skip adding edges!

  ROS_DEBUG_COND(cfg_->optim.optimization_verbose, "Adding velocity edges ...");
  std::size_t NoBandpts(teb_.sizePoses());
  Eigen::Matrix<double,2,2> information;
  information.fill(0);
  information(0,0) = cfg_->optim.weight_max_vel_x;
  information(1,1) = cfg_->optim.weight_max_vel_theta;

  for (std::size_t i=0; i < NoBandpts - 1; ++i)
  {
    EdgeVelocity* velocity_edge = new EdgeVelocity;
    velocity_edge->setVertex(0,teb_.PoseVertex(i));
    velocity_edge->setVertex(1,teb_.PoseVertex(i+1));
    velocity_edge->setVertex(2,teb_.TimeDiffVertex(i));
    velocity_edge->setInformation(information);
    velocity_edge->setTebConfig(*cfg_);
    optimizer_->addEdge(velocity_edge);
  }
}

void TebOptimalPlanner::AddEdgesAcceleration()
{
  if (cfg_->optim.weight_acc_lim_x==0 && cfg_->optim.weight_acc_lim_theta==0) 
    return; // if weight equals zero skip adding edges!

  ROS_DEBUG_COND(cfg_->optim.optimization_verbose, "Adding acceleration edges ...");
  std::size_t NoBandpts(teb_.sizePoses());
  Eigen::Matrix<double,2,2> information;
  information.fill(0);
  information(0,0) = cfg_->optim.weight_acc_lim_x;
  information(1,1) = cfg_->optim.weight_acc_lim_theta;
  
  // check if an initial velocity should be taken into accound
  if (vel_start_.first)
  {
    EdgeAccelerationStart* acceleration_edge = new EdgeAccelerationStart;
    acceleration_edge->setVertex(0,teb_.PoseVertex(0));
    acceleration_edge->setVertex(1,teb_.PoseVertex(1));
    acceleration_edge->setVertex(2,teb_.TimeDiffVertex(0));
    acceleration_edge->setInitialVelocity(vel_start_.second);
    acceleration_edge->setInformation(information);
    acceleration_edge->setTebConfig(*cfg_);
    optimizer_->addEdge(acceleration_edge);
  }

  // now add the usual acceleration edge for each tuple of three teb poses
  for (std::size_t i=0; i < NoBandpts - 2; ++i)
  {
    EdgeAcceleration* acceleration_edge = new EdgeAcceleration;
    acceleration_edge->setVertex(0,teb_.PoseVertex(i));
    acceleration_edge->setVertex(1,teb_.PoseVertex(i+1));
    acceleration_edge->setVertex(2,teb_.PoseVertex(i+2));
    acceleration_edge->setVertex(3,teb_.TimeDiffVertex(i));
    acceleration_edge->setVertex(4,teb_.TimeDiffVertex(i+1));
    acceleration_edge->setInformation(information);
    acceleration_edge->setTebConfig(*cfg_);
    optimizer_->addEdge(acceleration_edge);
  }
  
  // check if a goal velocity should be taken into accound
  if (vel_goal_.first)
  {
    EdgeAccelerationGoal* acceleration_edge = new EdgeAccelerationGoal;
    acceleration_edge->setVertex(0,teb_.PoseVertex(NoBandpts-2));
    acceleration_edge->setVertex(1,teb_.PoseVertex(NoBandpts-1));
    acceleration_edge->setVertex(2,teb_.TimeDiffVertex( teb_.sizeTimeDiffs()-1 ));
    acceleration_edge->setGoalVelocity(vel_goal_.second);
    acceleration_edge->setInformation(information);
    acceleration_edge->setTebConfig(*cfg_);
    optimizer_->addEdge(acceleration_edge);
  }  
}



void TebOptimalPlanner::AddEdgesTimeOptimal()
{
  if (cfg_->optim.weight_optimaltime==0) 
    return; // if weight equals zero skip adding edges!

  ROS_DEBUG_COND(cfg_->optim.optimization_verbose, "Adding edges for optimal Time ...");
  Eigen::Matrix<double,1,1> information;
  information.fill(cfg_->optim.weight_optimaltime);

  for (std::size_t i=0; i < teb_.sizeTimeDiffs(); ++i)
  {
    EdgeTimeOptimal* timeoptimal_edge = new EdgeTimeOptimal;
    timeoptimal_edge->setVertex(0,teb_.TimeDiffVertex(i));
    timeoptimal_edge->setInformation(information);
    timeoptimal_edge->setTebConfig(*cfg_);
    optimizer_->addEdge(timeoptimal_edge);
  }
}



void TebOptimalPlanner::AddEdgesKinematics()
{
  if (cfg_->optim.weight_kinematics_nh==0 && cfg_->optim.weight_kinematics_forward_drive==0)
    return; // if weight equals zero skip adding edges!
  
  ROS_DEBUG_COND(cfg_->optim.optimization_verbose, "Adding edges for kinematic constraints ...");

  // create edge for satisfiying kinematic constraints
  Eigen::Matrix<double,2,2> information_kinematics;
  information_kinematics.fill(0.0);
  information_kinematics(0, 0) = cfg_->optim.weight_kinematics_nh;
  information_kinematics(1, 1) = cfg_->optim.weight_kinematics_forward_drive;
  
  for (unsigned int i=0; i < teb_.sizePoses()-1; i++) // ignore twiced start only
  {
    EdgeKinematics* kinematics_edge = new EdgeKinematics;
    kinematics_edge->setVertex(0,teb_.PoseVertex(i));
    kinematics_edge->setVertex(1,teb_.PoseVertex(i+1));      
    kinematics_edge->setInformation(information_kinematics);
    kinematics_edge->setTebConfig(*cfg_);
    optimizer_->addEdge(kinematics_edge);
  }	 
}



void TebOptimalPlanner::computeCurrentCost(bool alternative_time_cost)
{ 
  // check if graph is empty/exist  -> important if function is called between buildGraph and optimizeGraph/clearGraph
  bool graph_exist_flag(false);
  if (optimizer_->edges().empty() && optimizer_->vertices().empty())
  {
    // here the graph is build again, for time efficiency make sure to call this function 
    // between buildGraph and Optimize (deleted), but it depends on the application
    buildGraph();	
    optimizer_->initializeOptimization();
  }
  else
  {
    graph_exist_flag = true;
  }
  
  optimizer_->computeInitialGuess();
  
  // now we need pointers to all edges -> calculate error for each edge-type
  // since we aren't storing edge pointers, we need to check every edge
    
  cost_.fill(0.0);
  
  if (alternative_time_cost)
  {
    cost_.coeffRef(0) = teb_.getSumOfAllTimeDiffs()*4; // normalize to cost magnitude! Value is obtained from a few measurements!
    //TEST we use SumOfAllTimeDiffs() here, because edge cost depends on number of samples, which is not always the same for similar TEBs,
    // since we are using an AutoResize Function with hysteresis. Unfortunately the sparse optimal_time edge cannot be normalized with the number of states.
  }
  
  for (std::vector<g2o::OptimizableGraph::Edge*>::const_iterator it = optimizer_->activeEdges().begin(); it!= optimizer_->activeEdges().end(); it++)
  {
    EdgeTimeOptimal* edge_time_optimal = dynamic_cast<EdgeTimeOptimal*>(*it);
    if (edge_time_optimal!=NULL)
    {
      if (!alternative_time_cost) 
	cost_.coeffRef(0) += pow(edge_time_optimal->getError()[0],2);
      continue;
    }

    EdgeKinematics* edge_kinematics = dynamic_cast<EdgeKinematics*>(*it);
    if (edge_kinematics!=NULL)
    {
      cost_.coeffRef(1) += pow(edge_kinematics->getError()[0],2);
      cost_.coeffRef(2) += pow(edge_kinematics->getError()[1],2);
      continue;
    }
    
    EdgeVelocity* edge_velocity = dynamic_cast<EdgeVelocity*>(*it);
    if (edge_velocity!=NULL)
    {
      cost_.coeffRef(3) += pow(edge_velocity->getError()[0],2);
      cost_.coeffRef(4) += pow(edge_velocity->getError()[1],2);
      continue;
    }
    
    EdgeAcceleration* edge_acceleration = dynamic_cast<EdgeAcceleration*>(*it);
    if (edge_acceleration!=NULL)
    {
      cost_.coeffRef(5) += pow(edge_acceleration->getError()[0],2);
      cost_.coeffRef(6) += pow(edge_acceleration->getError()[1],2);
      continue;
    }
    
    EdgePointObstacle* edge_point_obstacle = dynamic_cast<EdgePointObstacle*>(*it);
    if (edge_point_obstacle!=NULL)
    {
      cost_.coeffRef(7) += pow(edge_point_obstacle->getError()[0],2);
      continue;
    }

    EdgeLineObstacle* edge_line_obstacle = dynamic_cast<EdgeLineObstacle*>(*it);
    if (edge_line_obstacle!=NULL)
    {
      cost_.coeffRef(8) += pow(edge_line_obstacle->getError()[0],2);
      continue;
    }
    
    EdgePolygonObstacle* edge_polygon_obstacle = dynamic_cast<EdgePolygonObstacle*>(*it);
    if (edge_polygon_obstacle!=NULL)
    {
      cost_.coeffRef(9) += pow(edge_polygon_obstacle->getError()[0],2);
      continue;
    }  
  }
  
  // delete temporary created graph
  if (!graph_exist_flag) 
    clearGraph();
}


Eigen::Vector2d TebOptimalPlanner::getVelocityCommand() const
{
  if (teb_.sizePoses()<2)
    ROS_ERROR("TebOptimalPlanner::getVelocityCommand(): The trajectory contains less than 2 poses. Make sure to init and optimize/plan the trajectory fist.");
  
  double dt = teb_.TimeDiff(0);
  if (dt<=0)
  {	
    ROS_ERROR("TebOptimalPlanner::getVelocityCommand() - timediff<=0 is invalid!");
    return Eigen::Vector2d::Zero();			
  }
	  
  // Get velocity from the first two configurations
  Eigen::Vector2d vel;
  Eigen::Vector2d deltaS = teb_.Pose(1).position()-teb_.Pose(0).position();
  Eigen::Vector2d conf1dir( cos(teb_.Pose(0).theta()) , sin(teb_.Pose(0).theta()) );
  // translational velocity
  double dir = deltaS.dot(conf1dir);
  vel.coeffRef(0) = (double) g2o::sign(dir) * deltaS.norm()/dt;
  
  // rotational velocity
  double orientdiff = g2o::normalize_theta(teb_.Pose(1).theta()-teb_.Pose(0).theta());
  vel.coeffRef(1) = orientdiff/dt;
  
  return vel;
}

bool TebOptimalPlanner::isTrajectoryFeasible(base_local_planner::CostmapModel* costmap_model, const std::vector<geometry_msgs::Point>& footprint_spec,
                                             double inscribed_radius, double circumscribed_radius, int look_ahead_idx)
{
  if (look_ahead_idx < 0 || look_ahead_idx >= (int) teb().sizePoses())
    look_ahead_idx = (int) teb().sizePoses()-1;
  
  for (unsigned int i=0; i <= look_ahead_idx; ++i)
  {           
    if ( costmap_model->footprintCost(teb().Pose(i).x(), teb().Pose(i).y(), teb().Pose(i).theta(), footprint_spec, inscribed_radius, circumscribed_radius) < 0 )
      return false;
  }
  return true;
}

} // namespace teb_local_planner
