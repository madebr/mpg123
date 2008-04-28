#!/usr/bin/ruby
#
# Ruby control interface for mpg123
# By Nicholas Humfrey
#
# With thanks to Marc Lehmann (author of perl module Audio::Play::MPG123)
#

require "open3"

class Mpg123
	attr_reader :version, :state, :info, :error
	attr_reader :frame, :time
	
	STATE_STOPPED = 0
	STATE_PAUSED = 1
	STATE_PLAYING = 2

	def initialize(args=Array.new)
	
		## Set instance variable defaults
		@version = nil
		@state = STATE_STOPPED
		@info = Hash.new
		@error = nil	
		@frame = 0	
		@time = 0.0
		
		## Open pipe to mpg123 proccess
		cmd = ['mpg123','--remote', *args]
		@pipe_in, @pipe_out, pipe_err = Open3.popen3(*cmd)
		## FIXME: redirect pipe_err to stderr
		## FIXME: check for errors
		
		# Read in the first line from mpg123
    parse_line( readline_nonblock(1.0) )
    ## FIXME: ensure that @version is non-nil
    	
    # Start the reader thread
    @thread = Thread.new {
      loop do
        line = @pipe_out.gets
        if line.nil?
          $stderr.puts "Reached EOF when reading from mpg123 pipe."
          raise("Reached EOF when reading from mpg123 pipe.")
        else
          parse_line(line)
        end
      end
    }
	end

  def self.finalize(id)
    ## FIXME: cleanup mpg123 process and thread
    puts "Mpg123.finalize(#{id})"
  end
  
  def poll(timeout=0.1)
    line = readline( timeout )
    return if line == nil
    
    begin
      parse_line( line )
      # Check for more lines (without a timeout)
      line = readline(0)
    end until line == nil
  end
  
  def load(path)
    @pipe_in.puts("LOAD #{path}")
  end
  
  def loadpause(path)
    @pipe_in.puts("LOADPAUSE #{path}")
  end
  
  def pause
    @pipe_in.puts("PAUSE")
  end
  
  def paused?
   return (@state==STATE_PAUSED)
  end
  
  def playing?
   return (@state==STATE_PLAYING)
  end
  
  def stopped?
    return (@state==STATE_STOPPED)
  end
  
  def stop
    @pipe_in.puts("STOP")
  end
  
  def jump(frame)
    @pipe_in.puts("JUMP #{frame}")
  end
  
  def volume(percent)
    @pipe_in.puts("VOLUME #{percent}")
  end
  
  def rva(mode)
    @pipe_in.puts("RVA #{mode}")
  end
  
  def eq(channel, band, value)
    @pipe_in.puts("EQ #{channel} #{band} #{value}")
  end
  
  def seek(sample)
    @pipe_in.puts("SEEK #{sample}")
  end
  
  def simple_eq(bass,mid,treble)
    @pipe_in.puts("SEQ #{bass} #{mid} #{treble}")
  end



private
	
	## Parse a line sent to us from mpg123
	def parse_line(line)

		if (line =~ /^@F (\d+) (\d+) ([\d\.]+) ([\d\.]+)$/)
			@frame = $1.to_i
			#FIXME: @frames_remaining = $2.to_i
			@time = $3.to_f
			#FIXME: @time_remaining = $4.to_f
		elsif (line =~ /^@S ([\d\.]+) (\d+) (\d+) (.+)/)
			@info['mpeg_version'] = $1
			@info['layer'] = $2
			@info['sample_rate'] = $3
			@info['mode'] = $4
      ## FIXME: more available:
      #@{$self}{qw(type layer samplerate mode mode_extension
      #         bpf channels copyrighted error_protected
      #         emphasis bitrate extension lsf)}=split /\s+/,$1;
      @state = STATE_PLAYING
    elsif (line =~ /^@I ID3:(.{30})(.{30})(.{30})(....)(.{30})(.*)$/)
			@info['title']=$1;   @info['artist']=$2;
			@info['album']=$3;   @info['year']=$4;
			@info['comment']=$5; @info['genre']=$6;
		elsif (line =~ /^@I ICY-META: StreamTitle='(.*)';$/)
			@info['title']=$1;
			#delete @{$self}{qw(artist album year comment genre)}
		elsif (line =~ /^@I (.*)$/)
			@info['title']=$1;
			#delete @{$self}{qw(artist album year comment genre)}
		elsif (line =~ /^@R (.+)/)
			@version = $1
		elsif (line =~ /^@P (\d)/)
			@state = $1.to_i
		elsif (line =~ /^@E (.*)$/)
			@error = $1
			$stderr.puts("Error: #{$1}")
			raise("Error: #{$1}")
		elsif (line =~ /^@/)
			$stderr.puts "unrecognised line: #{line}"
		else
			$stderr.puts "unrecognised data: #{line}"
		end         
	end
	
	def readline_nonblock(timeout)
    # Is there any data available?
    unless IO.select([@pipe_out], nil, nil, timeout).nil?
      # Yes; read the line in and parse it
      return @pipe_out.gets
    else
      # No; nothing available
      return nil
		end
	end

end
